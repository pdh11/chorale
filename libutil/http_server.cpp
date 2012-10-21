#include "http_server.h"
#include "config.h"
#include "buffer_chain.h"
#include "file.h"
#include "file_stream.h"
#include "http_client.h"
#include "http_fetcher.h"
#include "http_parser.h"
#include "line_reader.h"
#include "partial_stream.h"
#include "poll.h"
#include "socket.h"
#include "errors.h"
#include "string_stream.h"
#include "trace.h"
#include "worker_thread_pool.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

#undef IN
#undef OUT

namespace util {

namespace http {

/** Per-socket HTTP server subtask
 */
class Server::Task: public util::PollableTask
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
    std::string m_headers;

    enum { BUFFER_SIZE = 8192 };
    boost::scoped_array<char> m_buffer;
    size_t m_buffer_fill;

    struct {
	bool closing;
	bool do_range;
	unsigned long long range_min;
	unsigned long long range_max;
	unsigned long long post_body_length;

	void Clear()
        {
	    closing = do_range = false; 
	    post_body_length = 0;
	}
    } m_entity;

    enum {
	WAITING,
	RECV_HEADERS,
	RECV_BODY,
	SEND_HEADERS,
	SEND_BODY
    } m_state;

public:
    Task(Server *parent, StreamSocketPtr client);
    ~Task();

    // Being a Task
    unsigned int Run();
};

Server::Task::Task(Server *parent, StreamSocketPtr client)
    : PollableTask(&parent->m_task_poller),
      m_parent(parent),
      m_socket(client),
      m_count(0),
      m_line_reader(m_socket),
      m_parser(&m_line_reader),
      m_buffer_fill(0),
      m_state(WAITING)
{
//    TRACE << "st" << this << ": accepted fd "
//	  << client->GetReadHandle() << "\n";
    m_socket->SetNonBlocking(true);
    m_socket->SetTimeoutMS(0);
    m_rq.Clear();
    m_rq.local_ep = m_socket->GetLocalEndPoint();
    m_rs.Clear();
}

Server::Task::~Task()
{
//    TRACE << "~Task in state " << m_state << "\n";
}

unsigned int Server::Task::Run()
{
    if (!m_socket->IsOpen())
	return 0;
//    TRACE << "In WSTR with live socket " << m_socket->GetReadHandle()
//	  << " state " << m_state << "\n";

    unsigned int rc;

    switch (m_state)
    {
    case WAITING:
	rc = m_parser.GetRequestLine(&m_rq.verb, &m_rq.path, NULL);
	if (rc == EWOULDBLOCK)
	{
//	    TRACE << "st" << this << " going idle waiting for request " << m_socket->GetReadHandle() << "\n";
	    WaitForReadable(m_socket);
	    return 0;
	}
	if (rc)
	{
//	    TRACE << (void*)this << " unreadable " << rc
//		  << " (request " << m_count << ")\n";
	    return rc;
	}

//	TRACE << "Got request " << m_rq.verb << " " << m_rq.path << "\n";

	++m_count;
	m_entity.Clear();
	m_state = RECV_HEADERS;
	/* fall through */

    case RECV_HEADERS:
	for (;;)
	{
	    std::string key, value;
	    rc = m_parser.GetHeaderLine(&key, &value);
	    if (rc == EWOULDBLOCK)
	    {
//		TRACE << "Going idle waiting for header " << m_socket->GetReadHandle() << "\n";
		WaitForReadable(m_socket);
		return 0;
	    }
	    if (rc)
		return rc;

//	    TRACE << "Got header '" << key << "' = '" << value << "'\n";

	    if (!strcasecmp(key.c_str(), "Connection"))
	    {
		if (!strcasecmp(value.c_str(), "Close"))
		    m_entity.closing = true;
	    }
	    else if (!strcasecmp(key.c_str(), "Range"))
	    {
		if (sscanf(value.c_str(), "bytes=%llu-%llu", 
			   &m_entity.range_min,
			   &m_entity.range_max) == 2)
		{
		    m_entity.do_range = true;
		    ++m_entity.range_max; // HTTP is inc/inc; we want inc/exc
		}
		else if (sscanf(value.c_str(), "bytes=%llu-", 
				&m_entity.range_min) == 1)
		{
		    m_entity.do_range = true;
		    m_entity.range_max = (unsigned long long)-1; 
		    // Will be clipped against len later
		}
		else
		    TRACE << "Don't like range request '" << value
			  << "'\n";
	    }
	    else if (!strcasecmp(key.c_str(), "Cache-Control"))
		m_rq.refresh = true;
	    else if (!strcasecmp(key.c_str(), "Content-Length"))
	    {
		unsigned long long clen;
		if (sscanf(value.c_str(), "%llu", &clen) == 1)
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

	    if (key.empty())
		break;
	}
	
	if (m_entity.post_body_length)
	{
//	    TRACE << "Has body (" << m_entity.post_body_length << " bytes)\n";
	    m_rq.has_body = true;
	}
//	else
//	    TRACE << "No body\n";


//	TRACE << "st" << this << ": calling SFP\n";
	m_parent->StreamForPath(&m_rq, &m_rs);
//	TRACE << "st" << this << ": SFP returned\n";

	m_state = RECV_BODY;
	/* fall through */

    case RECV_BODY:
    {
	/** Peculiar phrasing here (rather than just referring directly
	 * to m_entity.post_body_length) because GCC 4.3.2 for MinGW
	 * appears to miscompile it such that the loop never exits.
	 */
	size_t remain = m_entity.post_body_length;

	while (remain)
	{
	    size_t readable;
	    rc = m_socket->GetReadable(&readable);
	    if (rc)
	    {
		TRACE << "Not readable" << rc << "\n";
		return rc;
	    }

	    if (!readable)
	    {
//		TRACE << "Going idle waiting for readable body\n";
		m_entity.post_body_length = remain;
		WaitForReadable(m_socket);
		return 0;
	    }

	    readable = std::min(readable, (size_t)4096);
	    readable = std::min(readable, remain);

	    BufferPtr ptr(new Buffer((bufsize_t)readable));

	    rc = m_socket->Read(ptr->data, readable, &readable);
	    ptr.len = (bufsize_t)readable;

//	    TRACE << "Actually read " << readable << "\n";

	    if (m_rs.body_sink.get())
		m_rs.body_sink->OnBuffer(ptr);

	    remain -= readable;
	}

	if (m_rs.body_sink.get())
	{
	    m_rs.body_sink->OnBuffer(BufferPtr(NULL));
	    m_rs.body_sink.reset(NULL);
	}

	m_headers.clear();
	if (!m_rs.ssp)
	{
	    m_headers = "HTTP/1.1 404 Not Found\r\n";
	    StringStreamPtr ssp = StringStream::Create();
	    ssp->str() = "<i>404, dude, it's just not there</i>";
	    m_rs.ssp = ssp;
	}
	else if (m_entity.do_range)
	    m_headers = "HTTP/1.1 206 OK But A Bit Partial\r\n";
	else
	    m_headers = "HTTP/1.1 200 OK\r\n";

	m_headers += "Server: " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";
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

	unsigned long long len;

	if (m_rs.ssp)
	{
	    len = m_rs.ssp->GetLength();

	    if (m_entity.do_range)
	    {
		if (m_entity.range_min > len)
		    m_entity.range_min = len;
		if (m_entity.range_max > len)
		    m_entity.range_max = len;

		/* This form of the Content-Range header is completely
		 * bogus (the '=' should be whitespace), but is what
		 * Receivers expect.
		 */
		m_headers += (boost::format("Content-Range: bytes=%llu-%llu\r\n")
			      % m_entity.range_min
			      % (m_entity.range_max-1)).str();
//		headers += (boost::format("Content-Range: bytes %llu-%llu/%llu\r\n")
//			    % range_min
//			    % (range_max-1)
//			    % len).str();

		if (m_entity.range_min > 0 || m_entity.range_max != len)
		    m_rs.ssp = util::CreatePartialStream(m_rs.ssp, 
							 m_entity.range_min,
							 m_entity.range_max);

		len = m_entity.range_max - m_entity.range_min;
	    }
	}
	else
	    len = 0;

	m_headers += (boost::format("Content-Length: %llu\r\n") % len).str();
	m_headers += "\r\n";

//	TRACE << "Response headers:\n" << m_headers;

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

	if (m_rs.ssp)
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
		    rc = m_rs.ssp->Read(m_buffer.get() + m_buffer_fill,
					BUFFER_SIZE - m_buffer_fill, &nread);

//		    TRACE << "st" << this << ": Read(" << (BUFFER_SIZE-m_buffer_fill) << ")=" << rc << "\n";

		    if (rc)
		    {
			if (rc == EWOULDBLOCK && m_buffer_fill == 0)
			{
			    PollHandle h = m_rs.ssp->GetReadHandle();
			    if (h == NOT_POLLABLE)
			    {
				TRACE << "** Problem, non-pollable stream returned EWOULDBLOCK\n";
				return rc;
			    }
//			    TRACE << "Waiting for body readability\n";
			    WaitForReadable(m_rs.ssp);
			    return 0;
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

//		TRACE << "Attempting to write " << m_buffer_fill << "\n";
		size_t nwrote = 0;

		rc = m_socket->Write(m_buffer.get(), m_buffer_fill, 
				     &nwrote);
//		TRACE << "st" << this << ": write returned " << rc << " nwrote=" << nwrote << "\n";
		if (rc)
		{
		    if (rc == EWOULDBLOCK)
		    {
//			TRACE << "Waiting for body writability" << m_socket->GetWriteHandle() << "\n";
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
	}
	else
	{
//	    TRACE << "st" << this << ": 404\n";
	}

//	TRACE << "st" << this << ": wrote stream\n";

	if (m_entity.closing)
	    return 0;

        m_rq.Clear();
	m_rs.Clear();

//	TRACE << "Going round again on same connection\n";

	m_state = WAITING;
	
//	TRACE << "st" << this << ": going idle waiting for next request " << m_socket->GetReadHandle() << "\n";
	WaitForReadable(m_socket);
	return 0;

    default:
	assert(false);
    }

    return 0;
}


        /* Server itself */


Server::Server(util::PollerInterface *poller, 
	       util::WorkerThreadPool *thread_pool)
    : m_poller(poller),
      m_thread_pool(thread_pool),
      m_server_socket(StreamSocket::Create()),
      m_port(0),
      m_task_poller(poller, m_thread_pool)
{
}

Server::~Server()
{
    m_poller->Remove(m_server_socket.get());
//    TRACE << "~Server\n";
}

unsigned Server::Init(unsigned short port)
{
    IPEndPoint ep = { IPAddress::ANY, port };
    unsigned int rc = m_server_socket->Bind(ep);
    if (rc != 0)
    {
	TRACE << "Server bind failed " << rc << "\n";
	return rc;
    }

    ep = m_server_socket->GetLocalEndPoint();
    
    m_server_socket->SetNonBlocking(true);
    m_server_socket->Listen();
    m_poller->Add(m_server_socket,
		  util::Bind<Server, &Server::OnActivity>(this),
		  PollerInterface::IN);

    TRACE << "http::Server got port " << ep.port << "\n";
    m_port = ep.port;
    return 0;
}

unsigned Server::OnActivity()
{
//    TRACE << "Got activity, trying to accept\n";

    StreamSocketPtr ssp;
    unsigned int rc = m_server_socket->Accept(&ssp);
    if (rc != 0)
    {
	TRACE << "Accept failed " << rc << "\n";
	return 0;
    }
    
    m_thread_pool->PushTask(TaskPtr(new Server::Task(this, ssp)));

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
	return true;
    }

    struct stat st;
    if (stat(path2.c_str(), &st) < 0)
    {
	TRACE << "Resolved file '" << path2 << "' not found\n";
	return true;
    }

    if (!S_ISREG(st.st_mode))
    {
	TRACE << "Resolved file '" << path2 << "' not a file\n";
	return true;
    }

    unsigned int rc = util::OpenFileStream(path2.c_str(), util::READ,
					   &rs->ssp);
    if (rc != 0)
    {
	TRACE << "Resolved file '" << path2 << "' won't open " << rc
	      << "\n";
	return true;
    }

//    TRACE << "Path '" << rq->path << "' is file '" << path2 << "'\n";
    return true;
}

} // namespace http

} // namespace util

#ifdef TEST

class FetchTask: public util::Task
{
    util::http::Client *m_client;
    std::string m_url;
    std::string m_contents;
    util::PollerInterface *m_poller;
    volatile bool m_done;
    unsigned int m_which;

public:
    FetchTask(util::http::Client *client, const std::string& url,
	      util::PollerInterface *poller, unsigned int which)
	: m_client(client), m_url(url), m_poller(poller), m_done(false),
	  m_which(which) {}

    unsigned int Run()
    {
//	TRACE << "Fetcher " << m_which << " running\n";
	util::http::Fetcher hc(m_client, m_url);
//	TRACE << "hf" << &hc << " is fetcher " << m_which << "\n";
	unsigned int rc = hc.FetchToString(&m_contents);
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
	return true;
    }
};

int main(int, char*[])
{
    util::ThreadUser tu;

    util::Poller poller;

    enum { NUM_CLIENTS = 20 };

    util::WorkerThreadPool threads(util::WorkerThreadPool::NORMAL, 4);
    util::WorkerThreadPool client_threads(util::WorkerThreadPool::NORMAL,
					  NUM_CLIENTS);

    util::http::Client hc;
    util::http::Server ws(&poller, &threads);

    unsigned rc = ws.Init();

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    assert(rc == 0);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::TaskPtr tasks[NUM_CLIENTS];
    FetchTask *fetches[NUM_CLIENTS];

    for (unsigned int i=0; i<NUM_CLIENTS; ++i)
    {
	fetches[i] = new FetchTask(&hc, url, &poller, i);
	
	tasks[i].reset(fetches[i]);
	client_threads.PushTask(tasks[i]);
    }

    time_t start = time(NULL);
    time_t finish = start+30;
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "polling for " << (finish-now)*1000 << "ms\n";
	    poller.Poll((unsigned)(finish-now)*1000);
	}
    } while (now < finish);

    for (unsigned int i=0; i<NUM_CLIENTS; ++i)
    {
	FetchTask *ft = fetches[i];

//    TRACE << "Got '" << ft->GetContents() << "' (len "
//	  << strlen(ft->GetContents().c_str()) << " sz "
//	  << ft->GetContents().length() << ")\n";

	assert(ft->GetContents() == "/zootle/wurdle.html");
    }

    return 0;
}

#endif
