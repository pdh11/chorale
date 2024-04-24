#include "http_client.h"
#include "config.h"
#include "errors.h"
#include "scheduler.h"
#include "bind.h"
#include "magic.h"
#include "counted_pointer.h"
#include "http.h"
#include "http_parser.h"
#include "trace.h"
#include "scanf64.h"
#include "printf.h"
#include "socket.h"
#include "line_reader.h"
#include <string.h>
#include <limits.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/scoped_array.hpp>
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

LOG_DECL(HTTP);
LOG_DECL(HTTP_CLIENT);

namespace util {

namespace http {


        /* util::http::Client::Task */


class Client::Task: public util::Task
{
    Client *m_parent;
    RecipientPtr m_target;
    std::string m_extra_headers;
    std::string m_body;
    const char *m_verb;
    util::IPEndPoint m_remote_endpoint;

    util::Scheduler *m_scheduler;
    util::StreamSocket m_socket;
    std::string m_host;
    std::string m_path;
    std::string m_headers;

    enum { BUFFER_SIZE = 8192 };
    boost::scoped_array<char> m_buffer;
    size_t m_buffer_fill;

    GreedyLineReader m_line_reader;
    Parser m_parser;

    enum {
	UNINITIALISED, // 0
	CONNECTING,
	SEND_HEADERS,
	SEND_BODY,  // 3
	WAITING,
	RECV_HEADERS,
	RECV_BODY, // 6
	IDLE
    } m_state;

    struct {
	size_t transfer_length;
	size_t total_length;
	bool is_range;
	bool got_length;
	bool connection_close;

	void Clear()
	{
	    transfer_length = total_length = 0;
	    is_range = got_length = connection_close = false;
	}
    } m_entity;

    typedef CountedPointer<Client::Task> TaskPtr;

    void WaitForWritable()
    {
	m_scheduler->WaitForWritable(
	    Bind(TaskPtr(this)).To<&Task::Run>(), 
	    m_socket.GetHandle());
    }

    void WaitForReadable()
    {
	m_scheduler->WaitForReadable(
	    Bind(TaskPtr(this)).To<&Task::Run>(), 
	    m_socket.GetHandle());
    }

    class CallbackTask: public util::Task
    {
	RecipientPtr m_target;
	unsigned int m_rc;

    public:
	CallbackTask(RecipientPtr target, unsigned rc)
	    : m_target(target),
	      m_rc(rc)
	{
	}

	unsigned int Run()
	{
	    m_target->OnDone(m_rc);
	    return 0;
	}
    };
    
    void SendDone(unsigned int rc)
    {
	m_scheduler->Wait(
	    Bind(util::TaskPtr(new CallbackTask(m_target,rc))).To<&util::Task::Run>(), 0, 0);
    }

public:
    Task(Client *parent,
	 util::Scheduler *scheduler,
	 RecipientPtr target,
	 const std::string& url,
	 const std::string& extra_headers,
	 const std::string& body,
	 const char *verb);
    ~Task();

    unsigned int Run();

    unsigned int Init();
};
 
Client::Task::Task(Client *parent,
		   util::Scheduler *scheduler,
		   RecipientPtr target,
		   const std::string& url,
		   const std::string& extra_headers,
		   const std::string& body,
		   const char *verb)
    : m_parent(parent),
      m_target(target),
      m_extra_headers(extra_headers),
      m_body(body),
      m_verb(verb),
      m_scheduler(scheduler),
      m_socket(),
      m_buffer_fill(0),
      m_line_reader(&m_socket),
      m_parser(&m_line_reader),
      m_state(UNINITIALISED)
{
    std::string host; // "http://ip:port"
    ParseURL(url, &host, &m_path);
    std::string hostonly;
    ParseHost(host, 80, &hostonly, &m_remote_endpoint.port);
    std::stringstream ss;
    ss << hostonly << ":" << m_remote_endpoint.port;
    m_host = ss.str();
    m_remote_endpoint.addr = util::IPAddress::Resolve(hostonly.c_str());
}

/** Start connecting the socket.
 *
 * This must be done after the constructor returns, as otherwise
 * there's a risk that the task will run to completion and delete
 * itself before anyone is ready for it to do so.
 */
unsigned int Client::Task::Init()
{
    if (m_remote_endpoint.addr.addr == 0)
	return ENOENT;
	
    m_state = CONNECTING;
    m_socket.SetNonBlocking(true);
    WaitForWritable();

    unsigned int rc = m_socket.Connect(m_remote_endpoint);
    if (rc && rc != EINPROGRESS && rc != EWOULDBLOCK && rc != EISCONN)
    {
	TRACE << "Connect failed: " << rc << "\n";
	m_scheduler->Remove(TaskPtr(this));
	return rc;
    }

    return 0;
}

Client::Task::~Task()
{
    LOG(HTTP_CLIENT) << "ct" << this << ": ~Client::Task(" << m_path << ") in state "
		     << m_state << "\n";
//    m_scheduler->Remove(m_socket.get());
    LOG(HTTP_CLIENT) << "~Client::Task" << " done\n";
}

unsigned int Client::Task::Run()
{
    /* Note that at every "return" site in this function, it must
     * either call WaitForReadable, WaitForWritable, or
     * target->OnDone. Otherwise the task will get forgotten and
     * deleted.
     */

    unsigned rc;

    LOG(HTTP_CLIENT) << "ct" << this << ": activity, state " << m_state << "\n";

    switch (m_state)
    {
    case CONNECTING:
	LOG(HTTP_CLIENT) << "Connecting\n";
	rc = m_socket.Connect(m_remote_endpoint);

	/* Windows rather delightfully, can return EINVAL here whether the
	 * connection has succeeded or not.
	 */
	if (rc == EINVAL)
	{
	    if (m_socket.IsWritable())
		rc = EISCONN;
	}

	if (rc && rc != EISCONN)
	{
	    if (rc == EINPROGRESS || rc == EWOULDBLOCK)
	    {
		LOG(HTTP_CLIENT) << "Connection in progress (" << rc << ")\n";
		WaitForWritable();
		return 0;
	    }

	    TRACE << "Connect says " << rc << "\n";
	    m_target->OnDone(rc);
	    return rc;
	}
	LOG(HTTP_CLIENT) << "Connected from "
			 << m_socket.GetLocalEndPoint().ToString() << "\n";

	/* Now we're connected, we can work out which of our IP
	 * addresses got used.
	 */
	m_target->OnEndPoint(m_socket.GetLocalEndPoint());

	m_headers = m_verb ? m_verb : (m_body.empty() ? "GET" : "POST");
	m_headers += " " + m_path + " HTTP/1.1\r\n"
	    "Host: " + m_host + "\r\n"
	    "Connection: close\r\n";
	m_headers += m_parent->m_useragent_header;
	m_headers += "Accept: */*\r\n";
	m_headers += m_extra_headers;
	if (!m_body.empty()) {
	    m_headers += util::Printf() << "Content-Length: " << m_body.size()
					<< "\r\n";
        }
	m_headers += "\r\n";
	m_state = SEND_HEADERS;

	LOG(HTTP_CLIENT) << "Sending headers:\n" << m_headers << "\n";

	/* fall through */

    case SEND_HEADERS:
    {
	LOG(HTTP_CLIENT) << "Sending headers and body\n";

	Socket::Buffer iovec[2];
	iovec[0].ptr = m_headers.c_str();
	iovec[0].len = m_headers.size();
	iovec[1].ptr = m_body.c_str();
	iovec[1].len = m_body.size();

	size_t nwrote;
	rc = m_socket.WriteV(iovec, 2, &nwrote);
	if (rc == 0)
	{
	    if (nwrote < m_headers.size())
	    {
		m_headers.erase(0,nwrote);
	    }
	    else
	    {
		m_body.erase(0, nwrote - m_headers.size());
		m_headers.clear();
	    }
	}
	else if (rc != EWOULDBLOCK)
	{
	    TRACE << "Write error " << rc << "\n";
	    m_target->OnDone(rc);
	    return rc;
	}

	if (!m_headers.empty())
	{
	    TRACE << "EWOULDBLOCK(sendheaders), waiting for write\n";
	    WaitForWritable();
	    return 0;
	}

//	TRACE << "Sent all headers\n";

	m_state = SEND_BODY;
    }
    /* fall through */

    case SEND_BODY:
	if (!m_body.empty()) /// @todo writev(headers, body)
	{
	    size_t nwrote;
	    rc = m_socket.Write(m_body.c_str(), m_body.size(), &nwrote);
	    if (rc == 0)
		m_body.erase(0, nwrote);
	    else if (rc != EWOULDBLOCK)
	    {
		TRACE << "Write error(body) " << rc << "\n";
		m_target->OnDone(rc);
		return rc;
	    }

	    if (!m_body.empty())
	    {
		WaitForWritable();
		return 0;
	    }
	}
	
	/** @todo Remove this once pooling implemented
	 *
	 * For some reason, enabling this kills all SOAP access to WMP12. So
	 * we remove it anyway.
	m_socket.ShutdownWrite();
	 */
	
	m_state = WAITING;
	/* fall through */

    case WAITING:
    {
	unsigned int http_code;

	rc = m_parser.GetResponseLine(&http_code, NULL);
	LOG(HTTP) << "GRL returned " << rc << "\n";
	if (rc)
	{
	    if (rc == EWOULDBLOCK)
	    {
		WaitForReadable();
		return 0;
	    }
	    m_target->OnDone(rc);
	    return rc;
	}

	m_entity.Clear();
	m_state = RECV_HEADERS;
    }
    /* fall through */

    case RECV_HEADERS:
    {
	for (;;)
	{
	    std::string key, value;
	    rc = m_parser.GetHeaderLine(&key, &value);
	    if (rc)
	    {
		if (rc == EWOULDBLOCK)
		{
		    WaitForReadable();
		    return 0;
		}
		m_target->OnDone(rc);
		return rc;
	    }

	    if (key.empty())
		break;

	    m_target->OnHeader(key, value);

	    if (!strcasecmp(key.c_str(), "Content-Range"))
	    {
		/* HTTP/1.1 (RFC2616) says "Content-Range: bytes X-Y/Z"
		 * but traditional Receiver servers send
		 * "Content-Range: bytes=X-Y"
		 */
		uint64_t rmin, rmax, elen = 0;

		if (util::Scanf64(value.c_str(), "bytes %llu-%llu/%llu",
				  &rmin, &rmax, &elen) == 3
		    || util::Scanf64(value.c_str(), "bytes=%llu-%llu/%llu",
				     &rmin, &rmax, &elen) == 3
		    || util::Scanf64(value.c_str(), "bytes %llu-%llu",
				     &rmin, &rmax) == 2
		    || util::Scanf64(value.c_str(), "bytes=%llu-%llu",
				     &rmin, &rmax) == 2)
		{
		    if (elen)
			m_entity.total_length = (size_t)elen;

		    m_entity.transfer_length = (size_t)(rmax-rmin+1);
		    m_entity.is_range = true;
		    m_entity.got_length = true;
		}
	    }
	    else if (!strcasecmp(key.c_str(), "Content-Length"))
	    {
		uint64_t ull;
		util::Scanf64(value.c_str(), "%llu", &ull);
		m_entity.transfer_length = (size_t)ull;
		m_entity.got_length = true;
	    }
	    else if (!strcasecmp(key.c_str(), "Connection"))
	    {
		if (strstr(value.c_str(), "close"))
		    m_entity.connection_close = true;
	    }
	}

	if (!m_entity.got_length)
	{
	    m_entity.connection_close = true;
	    m_entity.transfer_length = UINT_MAX;
	}

	m_state = RECV_BODY;
    }
    /* fall through */

    case RECV_BODY:
    {
	/** @todo This is very like http::Server::Run's RECV_BODY -- merge
	          somehow?
	 */

	while (m_entity.transfer_length || m_buffer_fill)
	{
	    LOG(HTTP_CLIENT) << "transfer_length=" << m_entity.transfer_length
			     << " m_buffer_fill=" << m_buffer_fill << "\n";

	    if (!m_buffer)
	    {
		m_buffer.reset(new char[BUFFER_SIZE]);
		m_buffer_fill = 0;
	    }

	    size_t lump = std::min(m_entity.transfer_length, 
				   BUFFER_SIZE - m_buffer_fill);

	    if (lump)
	    {
		size_t nread;
		m_line_reader.ReadLeftovers(m_buffer.get() + m_buffer_fill,
					    lump, &nread);
		m_buffer_fill += nread;
		m_entity.transfer_length -= nread;
		lump -= nread;
//		TRACE << "Got " << nread << " bytes of body from leftovers\n";
	    }

	    if (lump)
	    {
		size_t nread;
		rc = m_socket.Read(m_buffer.get() + m_buffer_fill,
				    lump, &nread);
		if (rc == EWOULDBLOCK)
		{
		    if (m_buffer_fill == 0)
		    {
			WaitForReadable();
			return 0;
		    }
		}
		else if (rc)
		{
		    TRACE << "Read failed (" << rc << ")\n";
		    return rc;
		}
		else if (nread)
		{
		    LOG(HTTP_CLIENT) << "Read " << nread << " bytes\n";
		    m_buffer_fill += nread;
		    m_entity.transfer_length -= nread;
		}
		else
		{
		    // Success, no bytes -- must be EOF
		    TRACE << "EOF\n";
		    m_entity.transfer_length = 0;
		}
	    }

	    rc = m_target->OnData(m_buffer.get(), m_buffer_fill);
	    if (rc)
	    {
		TRACE << "Body write failed (" << rc << ")\n";
		return rc;
	    }
	    m_buffer_fill = 0;
	}

	LOG(HTTP_CLIENT) << "Done\n";

	SendDone(0);
	m_socket.Close();
	m_target.reset(NULL);
	m_entity.Clear();

	m_state = IDLE;
	/* fall through */
    }
    case IDLE:
//	TRACE << "HttpConnection going idle\n";
//	m_scheduler->Remove(m_socket.get());

	// -- When fixing, remove unconditional connection:close from headers
	//
	// if (!m_entity.connection_close)
	//     m_parent->PoolMe(this)

	break;

    case UNINITIALISED:
    default:
	assert(false);
	break;
    }
    return 0;
}

#if 0
unsigned int Client::Task::Read(void *buffer, size_t len, size_t *pread)
{
    assert(m_state != UNINITIALISED);

//    TRACE << "ct" << this << ": in Read()\n";
    if (m_state != RECV_BODY)
	return EWOULDBLOCK;

    if (m_entity.got_length && len > m_entity.transfer_length)
	len = m_entity.transfer_length;

    if (len == 0)
    {
	*pread = 0;
	return 0;
    }

    unsigned int rc = m_socket.Read(buffer, len, pread);
    if (rc)
    {
//	TRACE << "ct" << this << ": Read(" << m_socket
//	      << ", " << len << ") returned " << rc << "\n";
	return rc;
    }

//    TRACE << "ct" << this << ": read " << *pread << " of " << len
//	  << " adjusting tl " << m_entity.transfer_length << "\n";

    if (m_entity.got_length)
	m_entity.transfer_length -= *pread;
    else if (!*pread)
	m_entity.transfer_length = 0;

//    TRACE << "ct" << this << ": adjusted tl " << m_entity.transfer_length 
//	  << "\n";
    return 0;
}
#endif


        /* util::http::Client */


Client::Client()
{
    struct utsname ubuf;

    uname(&ubuf);

    m_useragent_header
	= util::Printf() << "User-Agent: "
			 << ubuf.sysname << "/" << ubuf.release
			 << " UPnP/1.0 " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";
}

unsigned int Client::Connect(util::Scheduler *scheduler,
			     RecipientPtr target,
			     const std::string& url,
			     const std::string& extra_headers,
			     const std::string& body,
			     const char *verb)
{
    util::CountedPointer<Task> ptr(new Client::Task(this, scheduler, target,
						    url, extra_headers, body,
						    verb));
    return ptr->Init();
}

} // namespace http

} // namespace util

#ifdef TEST

# include "http_server.h"
# include "poll.h"
# include "string_stream.h"
# include "worker_thread_pool.h"

class EchoContentFactory: public util::http::ContentFactory
{
public:
    bool StreamForPath(const util::http::Request *rq, util::http::Response *rs)
    {
	rs->body_source.reset(new util::StringStream(rq->path));
	return true;
    }
};

class TestObserver: public util::http::Recipient
{
    std::string m_reply;
    bool m_done;

public:
    TestObserver()
	: m_done(false)
    {
    }

    const std::string& GetReply() const { return m_reply; }

    void OnHeader(const std::string&, const std::string&)
    {
//	TRACE << "Header '" << key << "' = '" << value << "'\n";
    }
    
    unsigned OnData(const void *buffer, size_t len)
    {
	m_reply.append((const char*)buffer, len);
	return 0;
    }

    void OnDone(unsigned rc)
    {
	if (rc)
	{
	    TRACE << "HTTP error " << rc << " :(\n";
	}
	m_done = true;
    }    

    bool IsDone() const { return m_done; }
};

int main(int, char*[])
{
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);
    util::BackgroundScheduler scheduler;

    util::http::Server ws(&scheduler, &wtp);

    unsigned rc = ws.Init(0);

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    assert(rc == 0);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::http::Client client;

    util::CountedPointer<TestObserver> tobs(new TestObserver);

    rc = client.Connect(&scheduler, tobs, url);
    assert(rc == 0);

    time_t start = time(NULL);
    time_t finish = start+12; // Long enough for the worker thread to time out
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "polling for " << (finish-now)*1000 << "ms\n";
	    scheduler.Poll((unsigned)(finish-now)*1000);
	}
    } while (now < finish && !tobs->IsDone());

//    TRACE << "Got '" << tobs.GetReply() << "' (len "
//	  << strlen(tobs.GetReply().c_str()) << " sz "
//	  << tobs.GetReply().length() << ")\n";
    assert(tobs->GetReply() == "/zootle/wurdle.html");

    return 0;
}

#endif

