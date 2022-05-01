#include "http_server.h"
#include "config.h"
#include "file.h"
#include "bind.h"
#include "http.h"
#include "trace.h"
#include "socket.h"
#include "errors.h"
#include "printf.h"
#include "scanf64.h"
#include "scheduler.h"
#include "ip_filter.h"
#include "http_parser.h"
#include "file_stream.h"
#include "line_reader.h"
#include "string_stream.h"
#include "partial_stream.h"
#include "counted_pointer.h"
#include "worker_thread_pool.h"
#include <sys/stat.h>
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#if HAVE_WINDOWS_H
#include <windows.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

#undef IN
#undef OUT

LOG_DECL(HTTP);
LOG_DECL(HTTP_SERVER);

namespace util {

namespace http {

Response::Response()
    : content_type(NULL),
      status_line(NULL),
      length(0)
{
}

Response::~Response()
{
}

void Response::Clear()
{
    body_sink.reset(NULL);
    content_type = NULL;
    status_line = NULL;
    headers.clear();
    length = 0;
    body_source.reset(NULL);
}

/** Per-socket HTTP server subtask
 */
class Server::DataTask final: public util::Task
{
    Server *m_parent;
    std::unique_ptr<StreamSocket> m_socket;

    /** Count of requests serviced on this socket.
     */
    unsigned m_count;

    GreedyLineReader m_line_reader;
    Parser m_parser;

    Request m_rq;
    Response m_rs;
    std::string m_last_path;
    std::unique_ptr<util::Stream> m_response_stream;
    bool m_reuse_stream;

    std::string m_headers;

    enum { BUFFER_SIZE = 8192 };
    boost::scoped_array<char> m_buffer;
    size_t m_buffer_fill;

    struct {
	bool closing;
	bool do_range;
	uint64_t range_min;
	uint64_t range_max;
	uint64_t post_body_length;
	uint64_t total_written;

	void Clear()
        {
	    closing = do_range = false; 
	    range_min = range_max = post_body_length = total_written = 0;
	}
    } m_entity;

    enum {
	CHECKING,
	WAITING,
	RECV_HEADERS,
	RECV_BODY,
	SEND_HEADERS,
	SEND_BODY
    } m_state;

    typedef util::CountedPointer<DataTask> DataTaskPtr;

    void WaitForReadable(util::Stream* sock)
    {
	m_parent->m_scheduler->WaitForReadable(
	    Bind(DataTaskPtr(this)).To<&DataTask::OnActivity>(),
	    sock->GetHandle());
    }

    void WaitForWritable(util::Stream* sock)
    {
	m_parent->m_scheduler->WaitForWritable(
	    Bind(DataTaskPtr(this)).To<&DataTask::OnActivity>(),
	    sock->GetHandle());
    }

    /** Called from polling thread; punts work to background thread */
    unsigned OnActivity()
    {
	m_parent->m_pool->PushTask(
	    Bind(DataTaskPtr(this)).To<&DataTask::Run>());
	return 0;
    }

    DataTask(Server *parent, std::unique_ptr<StreamSocket> client);

public:
    ~DataTask();

    static TaskCallback Create(Server*, std::unique_ptr<StreamSocket>);

    /** Called on background thread */
    unsigned int Run() override;
};

TaskCallback Server::DataTask::Create(Server *parent,
				      std::unique_ptr<StreamSocket> client)
{
    DataTaskPtr ptr(new DataTask(parent, std::move(client)));
    return util::Bind(ptr).To<&DataTask::Run>();
}

Server::DataTask::DataTask(Server *parent, std::unique_ptr<StreamSocket> client)
    : m_parent(parent),
      m_socket(std::move(client)),
      m_count(0),
      m_line_reader(m_socket.get()),
      m_parser(&m_line_reader),
      m_reuse_stream(false),
      m_buffer_fill(0),
      m_state(CHECKING)
{
    LOG(HTTP_SERVER)
	<< "st" << this << ": accepted fd "
	<< client << "\n";
    m_socket->SetNonBlocking(true);
    m_rq.Clear();
    m_rq.local_ep = m_socket->GetLocalEndPoint();
    m_rs.Clear();
}

Server::DataTask::~DataTask()
{
//    TRACE << "~DataTask in state " << m_state << "\n";
}

unsigned int Server::DataTask::Run()
{
    if (!m_socket->IsOpen())
	return 0;

    LOG(HTTP_SERVER) << "In WSTR with live socket "
		     << m_socket << " state " << m_state
		     << "\n";

    unsigned int rc;

again:

    switch (m_state)
    {
    case CHECKING:
	if (m_parent->m_filter)
	{
	    IPEndPoint ipe = m_socket->GetRemoteEndPoint();
	    m_rq.access = m_parent->m_filter->CheckAccess(ipe.addr);
	    if (m_rq.access == util::IPFilter::DENY)
		return 0;
	}
	else
	    m_rq.access = util::IPFilter::FULL;

	m_state = WAITING;
	/* fall through */

    case WAITING:
    {
	std::string version;

	rc = m_parser.GetRequestLine(&m_rq.verb, &m_rq.path, &version);
	if (rc == EWOULDBLOCK)
	{
//	    TRACE << "st" << this << " going idle waiting for request " << m_socket << "\n";
	    WaitForReadable(m_socket.get());
	    return 0;
	}
	if (rc)
	{
	    LOG(HTTP_SERVER) << "hs" << this << " " << m_socket
			     << " unreadable " << rc << " (request "
			     << m_count << ")\n";
	    return rc;
	}

	++m_count;

	LOG(HTTP) << "Got request " << m_count << ": " << m_rq.verb << " "
		  << m_rq.path << " " << version << "\n";

	m_reuse_stream = (m_last_path == m_rq.path && m_rq.verb == "GET");
	if (m_reuse_stream)
	{
	    LOG(HTTP) << "Same path, keeping stream\n";
	}
	else
	{
	    LOG(HTTP) << "Last path '" << m_last_path << "' now '" << m_rq.path
		      << "' not reusing\n";
	    m_last_path = m_rq.path;
	}

	m_entity.Clear();

	if (version == "HTTP/1.0")
	    m_entity.closing = true;

	m_state = RECV_HEADERS;
    }
    /* fall through */

    case RECV_HEADERS:
	for (;;)
	{
	    std::string key, value;
	    rc = m_parser.GetHeaderLine(&key, &value);
	    if (rc == EWOULDBLOCK)
	    {
//		TRACE << "Going idle waiting for header " << m_socket << "\n";
		WaitForReadable(m_socket.get());
		return 0;
	    }
	    if (rc)
		return rc;

	    LOG(HTTP) << "Got header '" << key << "' = '" << value << "'\n";

	    if (key.empty())
		break;

	    if (!strcasecmp(key.c_str(), "Connection"))
	    {
		if (!strcasecmp(value.c_str(), "Close"))
		    m_entity.closing = true;
		else if (!strcasecmp(value.c_str(), "Keep-Alive"))
		    m_entity.closing = false;
	    }
	    else if (!strcasecmp(key.c_str(), "Range"))
	    {
		if (util::Scanf64(value.c_str(), "bytes=%llu-%llu",
				  &m_entity.range_min,
				  &m_entity.range_max) == 2)
		{
		    m_entity.do_range = true;
		    ++m_entity.range_max; // HTTP is inc/inc; we want inc/exc

		    LOG(HTTP_SERVER) << "Range: " << m_entity.range_min
				     << "-" << m_entity.range_max << "\n";
		}
		else if (util::Scanf64(value.c_str(), "bytes=%llu-", 
				       &m_entity.range_min) == 1)
		{
		    m_entity.do_range = true;
		    m_entity.range_max = (unsigned long long)-1; 
		    // Will be clipped against len later

		    LOG(HTTP_SERVER) << "Range: " << m_entity.range_min
				     << "-end\n";
		}
		else
		    TRACE << "Don't like range request '" << value
			  << "'\n";
	    }
	    else if (!strcasecmp(key.c_str(), "Cache-Control"))
	    {
		m_rq.refresh = true;
		m_reuse_stream = false;
	    }
	    else if (!strcasecmp(key.c_str(), "Content-Length"))
	    {
		uint64_t clen;
		if (util::Scanf64(value.c_str(), "%llu", &clen) == 1)
		{
		    m_entity.post_body_length = clen;
//		    TRACE << "clen " << clen << "\n";
		}
		else
		{
		    TRACE << "Unrecognisable content-length\n";
		}
	    }	    
	    else
		m_rq.headers[key] = value;
	}
	
	if (m_entity.post_body_length)
	{
//	    TRACE << "Has body (" << m_entity.post_body_length << " bytes)\n";
	    m_rq.has_body = true;
	}
//	else
//	    TRACE << "No body\n";

	if (!m_reuse_stream || !m_rs.body_source.get())
	{
	    m_rs.Clear();
	    LOG(HTTP) << "st" << this << " calling SFP\n";
	    m_parent->StreamForPath(&m_rq, &m_rs);
	    LOG(HTTP) << "st" << this << " SFP returned\n";
	}

	m_state = RECV_BODY;
	/* fall through */

    case RECV_BODY:
    {
	/** Peculiar phrasing here (rather than just referring directly
	 * to m_entity.post_body_length) because GCC 4.3.2 for MinGW
	 * appears to miscompile it such that the loop never exits.
	 */
	size_t remain = (size_t)m_entity.post_body_length;

	while (remain || (m_rs.body_sink.get() && m_buffer_fill))
	{
//	    TRACE << "remain=" << remain << " m_buffer_fill=" 
//		  << m_buffer_fill << "\n";

	    if (!m_buffer)
	    {
		m_buffer.reset(new char[BUFFER_SIZE]);
		m_buffer_fill = 0;
	    }

	    size_t lump = std::min(remain, BUFFER_SIZE - m_buffer_fill);

//	    TRACE << "lump=" << lump << "\n";

	    if (lump)
	    {
		size_t nread;
		m_line_reader.ReadLeftovers(m_buffer.get() + m_buffer_fill,
					    lump, &nread);
		m_buffer_fill += nread;
		remain -= nread;
		lump -= nread;
//		TRACE << "Got " << nread << " bytes of body from leftovers\n";
	    }

	    if (lump)
	    {
		size_t nread;
		rc = m_socket->Read(m_buffer.get() + m_buffer_fill,
				    lump, &nread);
//		TRACE << "Read returned " << rc << " nread=" << nread << "\n";
		if (rc == EWOULDBLOCK)
		{
		    if (m_buffer_fill == 0)
		    {
			m_entity.post_body_length = remain;
			WaitForReadable(m_socket.get());
			return 0;
		    }
		}
		else if (rc || nread == 0)
		{
		    TRACE << "Read failed (" << rc << ")\n";
		    return rc;
		}
		else
		{
		    m_buffer_fill += nread;
		    remain -= nread;
		}
	    }

	    if (m_rs.body_sink.get())
	    {
		size_t nwrote;

		rc = m_rs.body_sink->Write(m_buffer.get(), m_buffer_fill,
					   &nwrote);
		if (rc == EWOULDBLOCK)
		{
		    if (lump == 0)
		    {
			TRACE << "Waiting for body sink writability\n";
			m_entity.post_body_length = remain;
			WaitForWritable(m_rs.body_sink.get());
			return 0;
		    }
		}

		if (nwrote < m_buffer_fill)
		{
		    memmove(m_buffer.get(),
			    m_buffer.get() + nwrote, m_buffer_fill - nwrote);
		}
		m_buffer_fill -= nwrote;	   
	    }
	    else
	    {
		LOG(HTTP) << "No body sink -- discarding " << m_buffer_fill 
			  << " bytes\n";
		m_buffer_fill = 0;
	    }
	}

	// Signal successful completion with a 0-byte write
	if (m_rs.body_sink.get())
	    rc = m_rs.body_sink->Write(&remain, 0, &remain);

	m_rs.body_sink.reset(NULL);
	m_headers.clear();

	uint64_t len;

	if (!m_rs.body_source.get())
	{
	    if (m_rs.status_line)
		m_headers = m_rs.status_line;
	    else
		m_headers = "HTTP/1.1 404 Not Found\r\n";
	    m_rs.body_source.reset(
		new StringStream("<i>404, dude, it's just not there</i>"));
	    m_rs.content_type = "text/html";
	    len = m_rs.body_source->GetLength();
	    m_entity.do_range = false;
	    TRACE << "404ing " << m_rq.path << "\n";
	}
	else
	{
	    len = m_rs.length ? m_rs.length : m_rs.body_source->GetLength();

	    if (m_entity.do_range)
	    {
		LOG(HTTP_SERVER) << "Clipping range " << m_entity.range_min
				 << "-" << m_entity.range_max
				 << " against len " << len << "\n";
		if (m_entity.range_min > len)
		    m_entity.range_min = len;
		if (m_entity.range_max > len)
		    m_entity.range_max = len;
	    
		LOG(HTTP_SERVER) << "Clipped range " << m_entity.range_min
				 << "-" << m_entity.range_max << "\n";
	    
		uint64_t new_len = m_entity.range_max - m_entity.range_min;

		if (new_len)
		    len = new_len;
		else
		{
		    LOG(HTTP) << "Range unsatisfiable\n";
		    m_entity.do_range = false;
		}
	    }

	    if (m_rs.status_line)
		m_headers = m_rs.status_line;
	    else
	    {
		if (m_entity.do_range)
		    m_headers = "HTTP/1.1 206 OK But A Bit Partial\r\n";
		else
		    m_headers = "HTTP/1.1 200 OK\r\n";
	    }
	}

//	TRACE << "Initial headers " << m_headers;
	
	time_t now = ::time(NULL);
	struct tm bdtime = *gmtime(&now);

	char timebuf[40];
	// 0123456789012345678901234567890123456789
	// Date: Mon, 02 Jun 1982 00:00:00 GMT..
	strftime(timebuf, sizeof(timebuf),
		 "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &bdtime);
	m_headers += timebuf;

	m_headers += m_parent->GetServerHeader();
	m_headers += "Accept-Ranges: bytes\r\n";
	m_headers += "Content-Type: ";
	    
	if (m_rs.content_type)
	    m_headers += m_rs.content_type;
	else
	    m_headers += "text/html";
	m_headers += "\r\n";

	for (std::map<std::string, std::string>::const_iterator i = m_rs.headers.begin();
	     i != m_rs.headers.end();
	     ++i)
	    m_headers += i->first + ": " + i->second + "\r\n";

	if (m_entity.do_range)
	{
	    /* This form of the Content-Range header is completely
	     * bogus (the '=' should be whitespace), but is what
	     * Receivers expect.
	     */
	    m_headers += util::Printf() << "Content-Range: bytes="
					<< m_entity.range_min << "-"
					<< (m_entity.range_max-1) << "\r\n";

	    if (m_entity.range_min > 0 || m_entity.range_max != len)
		m_response_stream = util::CreatePartialStream(m_rs.body_source.get(), 
							      m_entity.range_min,
							      m_entity.range_max);
	    else
		m_response_stream = std::move(m_rs.body_source);
	}
	else
	    m_response_stream = std::move(m_rs.body_source);

	m_response_stream->Seek(0);

	m_headers += util::Printf() << "Content-Length: " << len
				    << "\r\n" "\r\n";

	LOG(HTTP) << "Response headers:\n" << m_headers;

//	TRACE << "st" << this << ": len=" << len << "\n";

	m_state = SEND_HEADERS;
    }
    /* Fall through */

    case SEND_HEADERS:
	/* This is a separate step only if there isn't a body; if
	 * there is, we try and send it and the headers in a single
	 * packet, i.e. in a single operation.
	 */
	if (!m_response_stream.get() || m_rq.verb == "HEAD")
	{
	    do {
		size_t nwrote;
		rc = m_socket->Write(m_headers.c_str(), m_headers.length(),
				     &nwrote);
		if (rc == EWOULDBLOCK)
		{
//		TRACE << "Going idle waiting for header writability\n";
		    WaitForWritable(m_socket.get());
		    return 0;
		}
		if (rc)
		{
		    TRACE << "Writing headers failed " << rc << "\n";
		    return rc;
		}
		m_headers.erase(0, nwrote);
	    } while (!m_headers.empty());
	}

//	TRACE << "st" << this << ": wrote headers\n";

	m_state = SEND_BODY;
	/* fall through */

    case SEND_BODY:

	if (m_response_stream.get() && m_rq.verb != "HEAD")
	{
	    if (!m_buffer)
	    {
		m_buffer.reset(new char[BUFFER_SIZE]);
		m_buffer_fill = 0;
	    }

	    bool eof = false;
	    do {
		if (m_buffer_fill < BUFFER_SIZE)
		{
		    size_t nread;
		    rc = m_response_stream->Read(
			m_buffer.get() + m_buffer_fill,
			BUFFER_SIZE - m_buffer_fill, &nread);

//		    TRACE << "st" << this << ": Read(" << (BUFFER_SIZE-m_buffer_fill) << ")=" << rc << "\n";

		    if (rc)
		    {
			if (rc == EWOULDBLOCK && m_buffer_fill == 0
			    && m_headers.empty())
			{
			    // Quickly check for socket death first
			    rc = m_socket->Write(m_buffer.get(), 0, &nread);
			    if (rc)
			    {
				TRACE << "st" << this << " " << m_socket
				      << " went away " << rc << "\n";
				return 0;
			    }

			    int h = m_response_stream->GetHandle();
			    if (h == NOT_POLLABLE)
			    {
				TRACE << "st" << this << ": ** Problem, non-pollable stream returned EWOULDBLOCK\n";
				return rc;
			    }
			    LOG(HTTP) << "Waiting for body readability\n";
			    WaitForReadable(m_response_stream.get());
			    return 0;
			}

			if (rc != EWOULDBLOCK)
			{
			    TRACE << "Body error " << rc << "\n";
			    return rc;
			}
		    }
		    else
		    {
			m_buffer_fill += nread;
			if (!nread)
			    eof = true;
		    }
		}

		if (eof && !m_buffer_fill && m_headers.empty())
		    break;

		LOG(HTTP_SERVER) << "Attempting to write " << m_buffer_fill
				 << "\n";
		size_t nwrote = 0;

		if (m_headers.empty())
		{
		    rc = m_socket->Write(m_buffer.get(), m_buffer_fill, 
					 &nwrote);
		}
		else
		{
		    Socket::Buffer iovec[2];
		    iovec[0].ptr = m_headers.c_str();
		    iovec[0].len = m_headers.size();
		    iovec[1].ptr = m_buffer.get();
		    iovec[1].len = m_buffer_fill;
		    rc = m_socket->WriteV(iovec, 2, &nwrote);
		}
		LOG(HTTP_SERVER) << "st" << this << ": " << m_socket
				 << " write returned " << rc
				 << " nwrote=" << nwrote << "\n";

		if (rc)
		{
		    if (rc == EWOULDBLOCK)
		    {
			LOG(HTTP_SERVER) << "Waiting for body writability "
					 << m_socket << "\n";
			WaitForWritable(m_socket.get());
			return 0;
		    }
		    TRACE << "Writing stream failed " << rc << "\n";
		    return rc;
		}

		if (!m_headers.empty())
		{
		    size_t hsize = m_headers.size();
		    m_headers.erase(0, nwrote);
		    nwrote -= (hsize - m_headers.size());
		}

		if (nwrote && nwrote < m_buffer_fill)
		{
		    memmove(m_buffer.get(),
			    m_buffer.get() + nwrote, m_buffer_fill - nwrote);
		}
		m_buffer_fill -= nwrote;

		m_entity.total_written += nwrote;

//		TRACE << "eof=" << eof << " fill=" << m_buffer_fill << "\n";

	    } while (!eof || m_buffer_fill || !m_headers.empty());

	    m_buffer.reset(NULL);

	    LOG(HTTP) << "st" << this << ": wrote stream, "
		      << m_entity.total_written << " bytes\n";
	}
	else
	{
	    LOG(HTTP) << "st" << this << ": no body\n";
	}

	if (m_entity.closing)
	{
	    LOG(HTTP) << "st" << this << " closing connection now\n";
	    m_socket->Close();
	    return 0;
	}

//	TRACE << "Going round again on same connection\n";

	m_rq.Clear();

	m_state = WAITING;
	goto again;

    default:
	assert(false);
    }

    return 0;
}


        /** Server::AcceptorTask */


class Server::AcceptorTask: public util::Task
{
    Server *m_parent;
    StreamSocket m_socket;

    explicit AcceptorTask(Server *parent)
	: m_parent(parent)
    {}

public:
    typedef util::CountedPointer<AcceptorTask> AcceptorTaskPtr;

    static AcceptorTaskPtr Create(Server *parent)
    {
	return AcceptorTaskPtr(new AcceptorTask(parent));
    }

    ~AcceptorTask()
    {
	LOG(HTTP_SERVER) << "~AcceptorTask\n";
    }

    unsigned Init(unsigned short port, Scheduler *scheduler)
    {
	IPEndPoint ep = { IPAddress::ANY, port };
	unsigned int rc = m_socket.Bind(ep);
	if (rc != 0)
	{
	    TRACE << "Server bind failed " << rc << "\n";
	    return rc;
	}

	m_socket.SetNonBlocking(true);
	m_socket.Listen();

	scheduler->WaitForReadable(util::Bind(AcceptorTaskPtr(this)).To<&AcceptorTask::Run>(),
				   m_socket.GetHandle(), false);
	return 0;
    }

    unsigned short GetPort()
    {
	IPEndPoint ep = m_socket.GetLocalEndPoint();
	return ep.port;
    }

    unsigned Run()
    {
//	TRACE << "Got activity, trying to accept\n";

	std::unique_ptr<StreamSocket> ssp;
	unsigned int rc = m_socket.Accept(&ssp);
	if (rc == 0)
	{
	    m_parent->m_pool->PushTask(DataTask::Create(m_parent,
                                                        std::move(ssp)));
	}
	else
	{
	    TRACE << "Accept failed " << rc << "\n";
	    if (rc != EWOULDBLOCK)
		return rc;
	}
	return 0;
    }
};



        /* Server itself */


Server::Server(util::Scheduler *scheduler,
	       util::WorkerThreadPool *pool,
	       util::IPFilter *filter)
    : m_scheduler(scheduler),
      m_pool(pool),
      m_filter(filter),
      m_port(0)
{
#ifdef WIN32
    OSVERSIONINFO osvi;
    memset(&osvi, '\0', sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    m_server_header = (boost::format("Server: Windows/%u.%u UPnP/1.0 " 
				     PACKAGE_NAME "/" PACKAGE_VERSION "\r\n"
			   ) % osvi.dwMajorVersion % osvi.dwMinorVersion).str();

#else
    struct utsname ubuf;

    uname(&ubuf);

    m_server_header
	= util::Printf() << "Server: "
			 << ubuf.sysname << "/" << ubuf.release
			 << " UPnP/1.0 " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";
#endif

    LOG(HTTP_SERVER) << m_server_header;
}

Server::~Server()
{
    LOG(HTTP_SERVER) << "~Server calls shutdown\n";

    m_scheduler->Shutdown();
    m_pool->Shutdown();

    LOG(HTTP_SERVER) << "~Server\n";
}

unsigned Server::Init(unsigned short port)
{
    AcceptorTask::AcceptorTaskPtr atp = AcceptorTask::Create(this);
    unsigned rc = atp->Init(port, m_scheduler);
    if (rc)
	return rc;
    m_port = atp->GetPort();
    TRACE << "http::Server got port " << m_port << "\n";
    return 0;
}

unsigned short Server::GetPort()
{
    return m_port;
}

void Server::StreamForPath(const Request *rq, Response *rs)
{
    /// @bug Locking vs AddContentFactory

    for (list_t::iterator i = m_content.begin(); 
	 i != m_content.end();
	 ++i)
    {
	bool taken = (*i)->StreamForPath(rq, rs);
	if (taken)
	    return;
    }

    if (rq->verb == "OPTIONS")
    {
	/* We don't support any options, but send 200 and an empty
	 * body to say so.
	 */
	rs->body_source.reset(new util::StringStream());
    }
}

void Server::AddContentFactory(const std::string&, ContentFactory *cf)
{
    m_content.push_back(cf);
}


        /* FileContentFactory */


FileContentFactory::~FileContentFactory()
{
}

static const struct { const char *extension; const char *mimetype; }
    mimemap[] = {
	{ "ico", "image/x-icon" },
	{ "jpg", "image/jpeg" },
	{ "png", "image/png" },
	{ "xml", "text/xml" },
    };

static const char *ContentType(const std::string& path)
{
    std::string extension = GetExtension(path.c_str());

    for (unsigned int i=0; i<sizeof(mimemap)/sizeof(*mimemap); ++i)
    {
	if (extension == mimemap[i].extension)
	    return mimemap[i].mimetype;
    }
    return NULL;
}

bool FileContentFactory::StreamForPath(const Request *rq, Response *rs)
{
//    TRACE << "Request for page '" << rq->path << "'\n";
    if (strncmp(rq->path.c_str(), m_page_root.c_str(), m_page_root.length()))
    {
//	TRACE << "Not in my root '" << m_page_root << "'\n";
	return false;
    }

    std::string path2 = m_file_root
	+ std::string(rq->path, m_page_root.length());
    path2 = util::Canonicalise(path2);

    // Make sure it's still under the right root (no "/../" attacks)
    if (strncmp(path2.c_str(), m_file_root.c_str(), m_file_root.length()))
    {
	TRACE << "Resolved file '" << path2 << "' not in my root '" 
	      << m_file_root << "'\n";
	return false;
    }

    struct stat st;
    if (stat(path2.c_str(), &st) < 0)
    {
	LOG(HTTP) << "Resolved file '" << path2 << "' not found\n";
	return false;
    }

    if (!S_ISREG(st.st_mode))
    {
	TRACE << "Resolved file '" << path2 << "' not a file\n";
	return false;
    }

    unsigned int rc = util::OpenFileStream(path2.c_str(), util::READ,
					   &rs->body_source);
    if (rc != 0)
    {
	TRACE << "Resolved file '" << path2 << "' won't open " << rc
	      << "\n";
	return false;
    }

    rs->content_type = ContentType(path2);
    if (!rs->content_type)
	rs->content_type = ContentType(rq->path);

    LOG(HTTP) << "Path '" << rq->path << "' is file '" << path2 << "'\n";
    return true;
}

} // namespace http

} // namespace util

#ifdef TEST

# include "worker_thread_pool.h"
# include "http_client.h"
# include "http_fetcher.h"

class FetchTask: public util::Task
{
    util::http::Client *m_client;
    std::string m_url;
    std::string m_contents;
    util::Scheduler *m_poller;
    volatile bool m_done;
//    unsigned int m_which;

public:
    FetchTask(util::http::Client *client, const std::string& url,
	      util::Scheduler *poller, unsigned int)
	: m_client(client), m_url(url), m_poller(poller), m_done(false)
	  //m_which(which)
    {}

    unsigned int Run()
    {
//	TRACE << "Fetcher " << m_which << " running\n";
	util::http::Fetcher hc(m_client, m_url, "Range: bytes=0-\r\n");
//	TRACE << "hf" << &hc << " is fetcher " << m_which << "\n";
	unsigned int rc = hc.FetchToString(&m_contents);
	assert(rc == 0);
	m_poller->Wake();
//	TRACE << "Fetcher " << m_which << " done " << rc << "\n";

	m_done = true;
	return 0;
    }

    const std::string& GetContents() const { return m_contents; }

    bool IsDone() const { return m_done; }
};

class EchoContentFactory: public util::http::ContentFactory
{
public:
    bool StreamForPath(const util::http::Request *rq, util::http::Response *rs)
    {
	rs->body_source.reset(new util::StringStream(rq->path));
	return true;
    }
};

static bool EqualButForStars(const char *got, const char *pattern)
{
    while (*got && *pattern)
    {
	if (*pattern != '*' && *got != *pattern)
	{
	    return false;
	}
	if (*pattern != '*' || got[1] == pattern[1])
	    ++pattern;
	++got;
    }
    return !*got && !*pattern;
}

static void HttpTest(util::BackgroundScheduler *poller, unsigned short port,
		     const char *tx, const char *rx)
{
    util::StreamSocket ss;
    util::IPEndPoint ipe;
    ipe.addr = util::IPAddress::FromDottedQuad(127,0,0,1);
    ipe.port = port;
    unsigned rc = ss.Connect(ipe);
    assert(rc == 0);
    rc = ss.WriteAll(tx, strlen(tx));
    assert(rc == 0);

    ss.SetNonBlocking(true);

    std::string rxs;
    time_t start = time(NULL);
    time_t finish = start + 10;

    do {
	time_t now = time(NULL);
	if (now >= finish)
	    break;

	rc = poller->Poll(1000);
	assert(rc == 0);

	char buffer[1024];
	size_t nread;
	do {
	    rc = ss.Read(buffer, sizeof(buffer), &nread);
	    if (rc == 0)
	    {
		rxs += std::string(buffer, buffer+nread);
	    }
	} while (rc == 0 && nread > 0);
    } while (rxs.size() < strlen(rx));

    bool ok = EqualButForStars(rxs.c_str(), rx);
    if (!ok)
    {
	fprintf(stderr, "Expected:%s\nGot:%s\n", rx, rxs.c_str());
    }
    assert(ok);
}

int main(int, char*[])
{
    enum { NUM_CLIENTS = 100,
	   NUM_THREADS = 50     // Note Wine (maybe Windows too) can't select() on more than 64 fds
    };

    util::WorkerThreadPool server_threads(util::WorkerThreadPool::NORMAL, 4);
    util::WorkerThreadPool client_threads(util::WorkerThreadPool::NORMAL,
					  NUM_THREADS);

    util::http::Client hc;
    util::BackgroundScheduler poller;
    util::http::Server ws(&poller,&server_threads);

    unsigned rc = ws.Init();

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    assert(rc == 0);

    // Simple test
    HttpTest(&poller,
	     ws.GetPort(),
	     "GET / HTTP/1.1\r\n"
	     "\r\n",
	     "HTTP/1.1 200 OK\r\n"
	     "Date: *\r\n"
	     "Server: * UPnP/1.0 chorale/*\r\n"
	     "Accept-Ranges: bytes\r\n"
	     "Content-Type: text/html\r\n"
	     "Content-Length: 1\r\n"
	     "\r\n"
	     "/");

    // Pipelining test
    HttpTest(&poller,
	     ws.GetPort(),
	     "GET /foo HTTP/1.1\r\n"
	     "\r\n"
	     "GET /bar HTTP/1.1\r\n"
	     "\r\n",

	     "HTTP/1.1 200 OK\r\n"
	     "Date: *\r\n"
	     "Server: * UPnP/1.0 chorale/*\r\n"
	     "Accept-Ranges: bytes\r\n"
	     "Content-Type: text/html\r\n"
	     "Content-Length: 4\r\n"
	     "\r\n"
	     "/foo"

	     "HTTP/1.1 200 OK\r\n"
	     "Date: *\r\n"
	     "Server: * UPnP/1.0 chorale/*\r\n"
	     "Accept-Ranges: bytes\r\n"
	     "Content-Type: text/html\r\n"
	     "Content-Length: 4\r\n"
	     "\r\n"
	     "/bar"
	);

    // Pipelining test with bodies
    HttpTest(&poller,
	     ws.GetPort(),
	     "POST /foo HTTP/1.1\r\n"
	     "Content-Length: 6\r\n"
	     "\r\n"
	     "ptang\n"
	     "POST /bar HTTP/1.1\r\n"
	     "Content-Length: 6\r\n"
	     "\r\n"
	     "frink\n",

	     "HTTP/1.1 200 OK\r\n"
	     "Date: *\r\n"
	     "Server: * UPnP/1.0 chorale/*\r\n"
	     "Accept-Ranges: bytes\r\n"
	     "Content-Type: text/html\r\n"
	     "Content-Length: 4\r\n"
	     "\r\n"
	     "/foo"

	     "HTTP/1.1 200 OK\r\n"
	     "Date: *\r\n"
	     "Server: * UPnP/1.0 chorale/*\r\n"
	     "Accept-Ranges: bytes\r\n"
	     "Content-Type: text/html\r\n"
	     "Content-Length: 4\r\n"
	     "\r\n"
	     "/bar"
	);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::CountedPointer<FetchTask> tasks[NUM_CLIENTS];

    for (unsigned int i=0; i<NUM_CLIENTS; ++i)
    {
	tasks[i].reset(new FetchTask(&hc, url, &poller, i));
	client_threads.PushTask(util::Bind(tasks[i]).To<&FetchTask::Run>());
    }

    time_t start = time(NULL);
    time_t finish = start+30;
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "polling for " << (finish-now)*1000 << "ms\n";
	    rc = poller.Poll((unsigned)(finish-now)*1000);
	    if (rc)
	    {
		TRACE << "Poll failed (" << rc << ")\n";
		exit(1);
	    }
	}

	unsigned int done = 0;
	for (unsigned int i=0; i<NUM_CLIENTS; ++i)
	{
	    if (tasks[i]->IsDone())
		++done;
	}
	if (done == NUM_CLIENTS)
	    break;

    } while (now < finish);

    for (unsigned int i=0; i<NUM_CLIENTS; ++i)
    {
	util::CountedPointer<FetchTask> ft = tasks[i];

//    TRACE << "Got '" << ft->GetContents() << "' (len "
//	  << strlen(ft->GetContents().c_str()) << " sz "
//	  << ft->GetContents().length() << ")\n";

	assert(ft->GetContents() == "/zootle/wurdle.html");
    }

    return 0;
}

#endif
