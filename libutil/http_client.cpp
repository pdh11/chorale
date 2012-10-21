#include "http_client.h"
#include "config.h"
#include "errors.h"
#include "poll.h"
#include "http_fetcher.h"
#include "bind.h"
#include "trace.h"
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

namespace util {

namespace http {


        /* util::http::Connection */


Connection::Connection(Client *parent,
		       util::PollerInterface *poller,
		       Connection::Observer *observer,
		       const std::string& url,
		       const std::string& extra_headers,
		       const std::string& body,
		       const char *verb)
    : m_parent(parent),
      m_observer(observer),
      m_extra_headers(extra_headers),
      m_body(body),
      m_verb(verb),
      m_poller(poller),
      m_socket(StreamSocket::Create()),
      m_line_reader(m_socket),
      m_parser(&m_line_reader),
      m_state(CONNECTING)
{
    ParseURL(url, &m_host, &m_path);
    std::string hostonly;
    ParseHost(m_host, 80, &hostonly, &m_remote_endpoint.port);
    m_remote_endpoint.addr = util::IPAddress::Resolve(hostonly.c_str());
}

/** Start connecting the socket.
 *
 * This must be done after the constructor returns, as otherwise our
 * OnActivity will start getting called, and its ConnectionPtr will
 * delete the connection unless our caller has already squirreled the result
 * of the constructor call away somewhere.
 */
void Connection::Init()
{
    if (m_remote_endpoint.addr.addr == 0)
    {
	m_observer->OnHttpDone(ENOENT);
	return;
    }
	
    m_socket->SetNonBlocking(true);
    m_socket->SetTimeoutMS(0);
    m_poller->Add(m_socket.get(),
		  Bind<Connection, &Connection::OnActivity>(this), 
		  util::PollerInterface::OUT);
    unsigned int rc = m_socket->Connect(m_remote_endpoint);
    if (rc && rc != EINPROGRESS && rc != EWOULDBLOCK)
    {
	TRACE << "Connect failed: " << rc << "\n";
	m_poller->Remove(m_socket.get());
	m_observer->OnHttpDone(rc);
    }
}

Connection::~Connection()
{
//    TRACE << "hc" << this << ": ~Connection in state " << m_state << "\n";
    m_poller->Remove(m_socket.get());
//    TRACE << "~Connection done\n";
}

unsigned int Connection::OnActivity()
{
    AssertValid();

    unsigned rc;

//    TRACE << "hc" << this << ": activity, state " << m_state << "\n";

    ConnectionPtr ptr(this); // Stop me being destroyed until OA exits

    switch (m_state)
    {
    case CONNECTING:
//	TRACE << "Connecting\n";
	rc = m_socket->Connect(m_remote_endpoint);
	if (rc && rc != EISCONN)
	{
	    if (rc == EINPROGRESS)
	    {
//		TRACE << "Connection in progress\n";
		return 0;
	    }

	    TRACE << "Connect says " << rc << "\n";
	    m_observer->OnHttpDone(rc);
	    return rc;
	}
//	TRACE << "Connected\n";

	/* Now we're connected, we can work out which of our IP
	 * addresses got used.
	 */
	m_local_endpoint = m_socket->GetLocalEndPoint();

	m_headers = m_verb ? m_verb : (m_body.empty() ? "GET" : "POST");
	m_headers += " " + m_path + " HTTP/1.1\r\n"
	    "Host: " + m_host + "\r\n"
	    "User-Agent: " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";
	if (!m_body.empty())
	    m_headers += (boost::format("Content-Length: %u\r\n") % m_body.size()).str();
	m_headers += m_extra_headers;
	m_headers += "\r\n";
	m_state = SEND_HEADERS;
	/* fall through */

    case SEND_HEADERS:
    {
//	TRACE << "Sending headers\n";

	size_t nwrote;
	rc = m_socket->Write(m_headers.c_str(), m_headers.length(), &nwrote);
	if (rc == 0)
	    m_headers.erase(0,nwrote);
	else if (rc != EWOULDBLOCK)
	{
	    TRACE << "Write error " << rc << "\n";
	    return rc;
	}
	if (!m_headers.empty())
	{
//	    TRACE << "EWOULDBLOCK(sendheaders), waiting for write\n";
	    return 0;
	}

//	TRACE << "Sent all headers\n";

	m_state = SEND_BODY;
	/* fall through */
    }

    case SEND_BODY:
	if (!m_body.empty())
	{
	    size_t nwrote;
	    rc = m_socket->Write(m_body.c_str(), m_body.size(), &nwrote);
	    if (rc == 0)
		m_body.erase(0, nwrote);
	    else if (rc != EWOULDBLOCK)
	    {
		TRACE << "Write error(body) " << rc << "\n";
		return rc;
	    }

	    if (!m_body.empty())
	    {
//		TRACE << "EWOULDBLOCK(sendbody), waiting for write\n";
		return 0;
	    }
	}

	m_poller->Remove(m_socket.get());
//	TRACE << "hc" << this << ": rebinding for read fd " << m_socket->GetReadHandle() << "\n";
	m_poller->Add(m_socket.get(), 
		      Bind<Connection,&Connection::OnActivity>(this), 
		      util::PollerInterface::IN);

	m_state = WAITING;
	/* fall through */

    case WAITING:
    {
	unsigned int http_code;

//	TRACE << "Waiting\n";

	rc = m_parser.GetResponseLine(&http_code, NULL);
//	TRACE << "GRL returned " << rc << "\n";
	if (rc)
	    return (rc == EWOULDBLOCK) ? 0 : rc;

	m_entity.Clear();
	m_state = RECV_HEADERS;
	/* fall through */
    }
	
    case RECV_HEADERS:
    {
	for (;;)
	{
	    std::string key, value;
	    rc = m_parser.GetHeaderLine(&key, &value);
	    if (rc)
		return (rc == EWOULDBLOCK) ? 0 : rc;

	    if (key.empty())
		break;

	    m_observer->OnHttpHeader(key, value);

	    if (!strcasecmp(key.c_str(), "Content-Range"))
	    {
		/* HTTP/1.1 (RFC2616) says "Content-Range: bytes X-Y/Z"
		 * but traditional Receiver servers send
		 * "Content-Range: bytes=X-Y"
		 */
		unsigned long long rmin, rmax, elen = 0;

		if (sscanf(value.c_str(), "bytes %llu-%llu/%llu",
			   &rmin, &rmax, &elen) == 3
		    || sscanf(value.c_str(), "bytes=%llu-%llu/%llu",
			      &rmin, &rmax, &elen) == 3
		    || sscanf(value.c_str(), "bytes %llu-%llu",
			      &rmin, &rmax) == 2
		    || sscanf(value.c_str(), "bytes=%llu-%llu",
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
		unsigned long long ull;
		sscanf(value.c_str(), "%llu", &ull);
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
	/* fall through */
    }

    case RECV_BODY:
//	TRACE << "hc" << this << ": in recv_body etl="
//	      << m_entity.transfer_length << " egl=" << m_entity.got_length
//	      << "\n";
	if (m_entity.transfer_length != 0 || !m_entity.got_length)
	{
//	    TRACE << "hc" << this << " calls ohd\n";
	    m_observer->OnHttpData();
	}
	if (m_entity.transfer_length != 0)
	{
//	    TRACE << "hc" << this << ": believe " << m_entity.transfer_length
//		  << " bytes remain\n";
	    return 0;
	}

	m_observer->OnHttpDone(0);

	m_state = IDLE;
	/* fall through */

    case IDLE:
//	TRACE << "HttpConnection going idle\n";
	m_poller->Remove(m_socket.get());
	// if (!m_entity.connection_close)
	//     m_parent->PoolMe(this)
	break;
	
    default:
	assert(false);
	break;
    }
    return 0;
}

unsigned int Connection::Read(void *buffer, size_t len, size_t *pread)
{
//    TRACE << "hc" << this << ": in Read()\n";
    if (m_state != RECV_BODY)
	return EWOULDBLOCK;

    if (m_entity.got_length && len > m_entity.transfer_length)
	len = m_entity.transfer_length;

    if (len == 0)
    {
	*pread = 0;
	return 0;
    }

    unsigned int rc = m_socket->Read(buffer, len, pread);
    if (rc)
    {
//	TRACE << "hc" << this << ": Read(fd " << m_socket->GetReadHandle()
//	      << ", " << len << ") returned " << rc << "\n";
	return rc;
    }

//    TRACE << "hc" << this << ": read " << *pread << " of " << len
//	  << " adjusting tl " << m_entity.transfer_length << "\n";

    if (m_entity.got_length)
	m_entity.transfer_length -= *pread;
    else if (!*pread)
	m_entity.transfer_length = 0;

//    TRACE << "hc" << this << ": adjusted tl " << m_entity.transfer_length 
//	  << "\n";
    return 0;
}


        /* util::http::Client */


Client::Client()
{
}

ConnectionPtr Client::Connect(util::PollerInterface *poller,
			      Connection::Observer *observer,
			      const std::string& url,
			      const std::string& extra_headers,
			      const std::string& body,
			      const char *verb)
{
    ConnectionPtr ptr(new Connection(this, poller, observer, url,
				     extra_headers, body, verb));
    ptr->Init();
    return ptr;
}

} // namespace http

} // namespace util

#ifdef TEST

# include "http_server.h"
# include "poll.h"
# include "string_stream.h"

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

class TestObserver: public util::http::Connection::Observer
{
    std::string m_reply;
    util::StreamPtr m_stream;
    bool m_done;

public:
    TestObserver()
	: m_done(false)
    {
    }

    void SetStream(util::StreamPtr stream)
    {
	m_stream = stream;
    }

    const std::string& GetReply() const { return m_reply; }

    unsigned OnHttpHeader(const std::string&, const std::string&)
    {
//	TRACE << "Header '" << key << "' = '" << value << "'\n";
	return 0;
    }
    
    unsigned OnHttpData()
    {
//	TRACE << "Data too\n";
	char buffer[1024];
	size_t nread;
	unsigned int rc = m_stream->Read(buffer, sizeof(buffer), &nread);
	if (rc)
	    return rc;
	m_reply.append(buffer, nread);
	return 0;
    }

    void OnHttpDone(unsigned rc)
    {
	if (rc)
	{
	    TRACE << "HTTP error " << rc << " :(\n";
	}
	m_done = true;
    }    

    bool Done() { return m_done; }
};

int main(int, char*[])
{
    util::ThreadUser tu;
    util::Poller poller;
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);

    util::http::Server ws(&poller, &wtp);

    unsigned rc = ws.Init(0);

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    assert(rc == 0);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::http::Client client;

    TestObserver tobs;

    util::http::ConnectionPtr connection = client.Connect(&poller, &tobs, url);

    tobs.SetStream(connection);

    time_t start = time(NULL);
    time_t finish = start+12; // Long enough for the worker thread to time out
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "polling for " << (finish-now)*1000 << "ms\n";
	    poller.Poll((unsigned)(finish-now)*1000);
	}
    } while (now < finish && !tobs.Done());

//    TRACE << "Got '" << tobs.GetReply() << "' (len "
//	  << strlen(tobs.GetReply().c_str()) << " sz "
//	  << tobs.GetReply().length() << ")\n";
    assert(tobs.GetReply() == "/zootle/wurdle.html");

    return 0;
}

#endif

