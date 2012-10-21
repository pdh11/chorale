#include "http_server.h"
#include "config.h"
#include "file.h"
#include "file_stream.h"
#include "http_client.h"
#include "http_fetcher.h"
#include "http_parser.h"
#include "ip_filter.h"
#include "line_reader.h"
#include "partial_stream.h"
#include "socket.h"
#include "errors.h"
#include "string_stream.h"
#include "peeking_line_reader.h"
#include "trace.h"
#include "scanf64.h"
#include "scheduler.h"
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

void Response::Clear()
{
    body_sink.reset(NULL);
    content_type = NULL;
    length = 0;
    ssp.reset(NULL);
}

/** Per-socket HTTP server subtask
 */
class Server::DataTask: public util::Task
{
    Server *m_parent;
    StreamSocketPtr m_socket;

    /** Count of requests serviced on this socket.
     */
    unsigned m_count;

    PeekingLineReader m_line_reader;
    Parser m_parser;

    Request m_rq;
    Response m_rs;
    std::string m_last_path;
    util::SeekableStreamPtr m_response_stream;
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

    void WaitForReadable(StreamPtr sock)
    {
	m_parent->m_scheduler->WaitForReadable(
	    Bind<DataTask,&DataTask::OnActivity>(DataTaskPtr(this)),
	    sock.get());
    }

    void WaitForWritable(StreamPtr sock)
    {
	m_parent->m_scheduler->WaitForWritable(
	    Bind<DataTask,&DataTask::OnActivity>(DataTaskPtr(this)),
	    sock.get());
    }

    /** Called from polling thread; punts work to background thread */
    unsigned OnActivity()
    {
	m_parent->m_pool->PushTask(
	    Bind<DataTask,&DataTask::Run>(DataTaskPtr(this)));
	return 0;
    }

    DataTask(Server *parent, StreamSocketPtr client);

public:
    ~DataTask();

    static TaskCallback Create(Server*, StreamSocketPtr);

    /** Called on background thread */
    unsigned int Run();
};

TaskCallback Server::DataTask::Create(Server *parent, StreamSocketPtr client)
{
    DataTaskPtr ptr(new DataTask(parent, client));
    return util::Bind<DataTask,&DataTask::Run>(ptr);
}

Server::DataTask::DataTask(Server *parent, StreamSocketPtr client)
    : m_parent(parent),
      m_socket(client),
      m_count(0),
      m_line_reader(m_socket),
      m_parser(&m_line_reader),
      m_reuse_stream(false),
      m_buffer_fill(0),
      m_state(CHECKING)
{
    LOG(HTTP_SERVER)
	<< "st" << this << ": accepted fd "
	<< client << "\n";
    m_socket->SetNonBlocking(true);
    m_socket->SetTimeoutMS(0);
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
	    WaitForReadable(m_socket);
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
	/* fall through */
    }
    case RECV_HEADERS:
	for (;;)
	{
	    std::string key, value;
	    rc = m_parser.GetHeaderLine(&key, &value);
	    if (rc == EWOULDBLOCK)
	    {
//		TRACE << "Going idle waiting for header " << m_socket << "\n";
		WaitForReadable(m_socket);
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


	if (!m_reuse_stream)
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

	while (remain || (m_rs.body_sink && m_buffer_fill))
	{
	    if (!m_buffer)
	    {
		m_buffer.reset(new char[BUFFER_SIZE]);
		m_buffer_fill = 0;
	    }

	    size_t lump = std::min(remain, BUFFER_SIZE - m_buffer_fill);

	    if (lump)
	    {
		size_t nread;
		rc = m_socket->Read(m_buffer.get() + m_buffer_fill,
				    lump, &nread);
		if (rc == EWOULDBLOCK)
		{
		    if (m_buffer_fill == 0)
		    {
			WaitForReadable(m_socket);
			return 0;
		    }
		}
		else if (rc)
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

	    if (m_rs.body_sink)
	    {
		size_t nwrote;

		rc = m_rs.body_sink->Write(m_buffer.get(), m_buffer_fill,
					   &nwrote);
		if (rc == EWOULDBLOCK)
		{
		    if (lump == 0)
		    {
			TRACE << "Waiting for body sink writability\n";
			WaitForWritable(m_rs.body_sink);
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
	}

	m_rs.body_sink.reset(NULL);
	m_headers.clear();

	uint64_t len;

	if (!m_rs.ssp)
	{
	    m_headers = "HTTP/1.1 404 Not Found\r\n";
	    StringStreamPtr ssp = StringStream::Create();
	    ssp->str() = "<i>404, dude, it's just not there</i>";
	    m_rs.ssp = ssp;
	    m_rs.content_type = "text/html";
	    len = ssp->GetLength();
	    m_entity.do_range = false;
	}
	else
	{
	    len = m_rs.length ? m_rs.length : m_rs.ssp->GetLength();

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

	    if (m_entity.do_range)
		m_headers = "HTTP/1.1 206 OK But A Bit Partial\r\n";
	    else
		m_headers = "HTTP/1.1 200 OK\r\n";
	}
	
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
	    m_headers += (boost::format("Content-Range: bytes=%llu-%llu\r\n")
			  % m_entity.range_min
			  % (m_entity.range_max-1)).str();
//	   headers += (boost::format("Content-Range: bytes %llu-%llu/%llu\r\n")
//			    % range_min
//			    % (range_max-1)
//			    % len).str();

	    if (m_entity.range_min > 0 || m_entity.range_max != len)
		m_response_stream = util::CreatePartialStream(m_rs.ssp, 
							  m_entity.range_min,
							  m_entity.range_max);
	    else
		m_response_stream = m_rs.ssp;
	}
	else
	    m_response_stream = m_rs.ssp;

	m_response_stream->Seek(0);

	m_headers += (boost::format("Content-Length: %llu\r\n") % len).str();
	m_headers += "\r\n";

	LOG(HTTP) << "Response headers:\n" << m_headers;

//	TRACE << "st" << this << ": len=" << len << "\n";

	m_state = SEND_HEADERS;
	/* Fall through */
    }

    case SEND_HEADERS:
	do {
	    size_t nwrote;
	    rc = m_socket->Write(m_headers.c_str(), m_headers.length(),
				 &nwrote);
	    if (rc == EWOULDBLOCK)
	    {
//		TRACE << "Going idle waiting for header writability\n";
		WaitForWritable(m_socket);
		return 0;
	    }
	    if (rc)
	    {
		TRACE << "Writing headers failed " << rc << "\n";
		return rc;
	    }
	    m_headers.erase(0, nwrote);
	} while (!m_headers.empty());

//	TRACE << "st" << this << ": wrote headers\n";

	m_state = SEND_BODY;
	/* fall through */

    case SEND_BODY:

	if (m_response_stream && m_rq.verb != "HEAD")
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
			if (rc == EWOULDBLOCK && m_buffer_fill == 0)
			{
			    // Quickly check for socket death first
			    rc = m_socket->Write(m_buffer.get(), 0, &nread);
			    if (rc)
			    {
				TRACE << "st" << this << " " << m_socket
				      << " went away " << rc << "\n";
				return 0;
			    }

			    PollHandle h = m_response_stream->GetHandle();
			    if (h == NOT_POLLABLE)
			    {
				TRACE << "st" << this << ": ** Problem, non-pollable stream returned EWOULDBLOCK\n";
				return rc;
			    }
			    LOG(HTTP) << "Waiting for body readability\n";
			    WaitForReadable(m_response_stream);
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

		if (eof && !m_buffer_fill)
		    break;

		LOG(HTTP_SERVER) << "Attempting to write " << m_buffer_fill
				 << "\n";
		size_t nwrote = 0;

		rc = m_socket->Write(m_buffer.get(), m_buffer_fill, 
				     &nwrote);
		LOG(HTTP_SERVER) << "st" << this << ": " << m_socket
				 << " write returned " << rc
				 << " nwrote=" << nwrote << "\n";

		m_entity.total_written += nwrote;

		if (rc)
		{
		    if (rc == EWOULDBLOCK)
		    {
			LOG(HTTP_SERVER) << "Waiting for body writability "
					 << m_socket << "\n";
			WaitForWritable(m_socket);
			return 0;
		    }
		    TRACE << "Writing stream failed " << rc << "\n";
		    return rc;
		}
		if (nwrote < m_buffer_fill)
		{
		    memmove(m_buffer.get(),
			    m_buffer.get() + nwrote, m_buffer_fill - nwrote);
		}
		m_buffer_fill -= nwrote;

//		TRACE << "eof=" << eof << " fill=" << m_buffer_fill << "\n";

	    } while (!eof || m_buffer_fill);

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
	
	LOG(HTTP) << "st" << this << ": going idle waiting for next request "
		  << m_socket << "\n";
	WaitForReadable(m_socket);
	return 0;

    default:
	assert(false);
    }

    return 0;
}


        /** Server::AcceptorTask */


class Server::AcceptorTask: public util::Task
{
    Server *m_parent;
    StreamSocketPtr m_socket;

    typedef CountedPointer<AcceptorTask> AcceptorTaskPtr;

    AcceptorTask(Server *parent, StreamSocketPtr socket)
	: m_parent(parent), m_socket(socket)
    {}

public:

    static TaskCallback Create(Server *parent, StreamSocketPtr socket)
    {
	AcceptorTaskPtr ptr(new AcceptorTask(parent, socket));
	return util::Bind<AcceptorTask,&AcceptorTask::Run>(ptr);
    }

    ~AcceptorTask()
    {
	LOG(HTTP_SERVER) << "~AcceptorTask\n";
    }

    unsigned Run()
    {
//	TRACE << "Got activity, trying to accept\n";

	StreamSocketPtr ssp;
	unsigned int rc = m_socket->Accept(&ssp);
	if (rc == 0)
	{
	    m_parent->m_pool->PushTask(DataTask::Create(m_parent, ssp));
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

    m_server_header = (boost::format("Server: Windows/%u.%u UPNP/1.0 " 
				     PACKAGE_NAME "/" PACKAGE_VERSION "\r\n"
			   ) % osvi.dwMajorVersion % osvi.dwMinorVersion).str();

#else
    struct utsname ubuf;

    uname(&ubuf);

    m_server_header = (boost::format("Server: %s/%s UPNP/1.0 " 
				     PACKAGE_NAME "/" PACKAGE_VERSION "\r\n"
			   ) % ubuf.sysname % ubuf.release).str();
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
    StreamSocketPtr server_socket(StreamSocket::Create());
    IPEndPoint ep = { IPAddress::ANY, port };
    unsigned int rc = server_socket->Bind(ep);
    if (rc != 0)
    {
	TRACE << "Server bind failed " << rc << "\n";
	return rc;
    }

    server_socket->SetNonBlocking(true);
    server_socket->Listen();

    m_scheduler->WaitForReadable(AcceptorTask::Create(this, server_socket),
				 server_socket.get(), false);

    ep = server_socket->GetLocalEndPoint();
    TRACE << "http::Server got port " << ep.port << "\n";
    m_port = ep.port;
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
}

void Server::AddContentFactory(const std::string&, ContentFactory *cf)
{
    m_content.push_back(cf);
}


        /* FileContentFactory */


FileContentFactory::~FileContentFactory()
{
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
					   &rs->ssp);
    if (rc != 0)
    {
	TRACE << "Resolved file '" << path2 << "' won't open " << rc
	      << "\n";
	return false;
    }

    LOG(HTTP) << "Path '" << rq->path << "' is file '" << path2 << "'\n";
    return true;
}

} // namespace http

} // namespace util

#ifdef TEST

#include "worker_thread_pool.h"

class FetchTask: public util::Task
{
    util::http::Client *m_client;
    std::string m_url;
    std::string m_contents;
    util::Scheduler *m_poller;
    volatile bool m_done;
    unsigned int m_which;

public:
    FetchTask(util::http::Client *client, const std::string& url,
	      util::Scheduler *poller, unsigned int which)
	: m_client(client), m_url(url), m_poller(poller), m_done(false),
	  m_which(which) {}

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
	util::StringStreamPtr ssp = util::StringStream::Create();
	ssp->str() = rq->path;
	rs->ssp = ssp;
//	TRACE << "returned stream for path " << rq->path << "\n";
	return true;
    }
};

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

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::CountedPointer<FetchTask> tasks[NUM_CLIENTS];

    for (unsigned int i=0; i<NUM_CLIENTS; ++i)
    {
	tasks[i].reset(new FetchTask(&hc, url, &poller, i));
	client_threads.PushTask(util::Bind<FetchTask,&FetchTask::Run>(tasks[i]));
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
