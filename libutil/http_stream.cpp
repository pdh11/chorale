#include "http_stream.h"
#include "config.h"
#include "http_client.h"
#include "http_fetcher.h"
#include "http_parser.h"
#include "partial_stream.h"
#include "string_stream.h"
#include "trace.h"
#include "errors.h"
#include <errno.h>
#include <string.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

namespace util {

namespace http {

Stream::Stream(const IPEndPoint& ipe, const std::string& host,
	       const std::string& path,
	       const char *extra_headers, const char *body)
    : m_ipe(ipe),
      m_host(host),
      m_path(path),
      m_extra_headers(extra_headers),
      m_body(body),
      m_socket(StreamSocket::Create()),
      m_len(0),
      m_need_fetch(true),
      m_last_pos(0)
{
}

Stream::~Stream()
{
}

unsigned Stream::Create(StreamPtr *result, const char *url,
			const char *extra_headers, const char *body)
{
    std::string host;
    IPEndPoint ipe;
    std::string path;
    ParseURL(url, &host, &path);
    std::string hostonly;
    ParseHost(host, 80, &hostonly, &ipe.port);
    ipe.addr = IPAddress::Resolve(hostonly.c_str());
    if (ipe.addr.addr == 0)
	return ENOENT;

    *result = StreamPtr(new Stream(ipe, host, path, extra_headers,
				   body));

    return 0;
}

unsigned Stream::ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread)
{
    /** @todo Isn't thread-safe like the other ReadAt's. Add a mutex. */

    if (!m_need_fetch && pos != m_last_pos)
    {
	if (m_body) // Don't re-POST
	{
	    TRACE << "Oh-oh, not re-POST-ing just because pos (" << pos
		  << ") != m_last_pos (" << m_last_pos << ")\n";
	    return EINVAL;
	}

	m_socket->Close();
	m_socket->Open();
	m_need_fetch = true;
	Seek(pos);
    }

    if (m_len && pos == m_len)
    {
	*pread = 0;
	return 0;
    }

    if (m_need_fetch)
    {
	m_socket->SetNonBlocking(false);

	unsigned int rc = m_socket->Connect(m_ipe);
	if (rc != 0)
	    return rc;

	std::string headers;
	if (m_body)
	    headers = "POST";
	else
	    headers = "GET";

	headers += " " + m_path + " HTTP/1.1\r\n";

	headers += "Host: " + m_host + "\r\n";

	if (pos)
	    headers += (boost::format("Range: bytes=%llu-\r\n")
			% (unsigned long long)pos).str();

	headers += "User-Agent: " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";

	if (m_body)
	    headers += (boost::format("Content-Length: %llu\r\n")
			% (unsigned long long)strlen(m_body)).str();

	if (m_extra_headers)
	    headers += m_extra_headers;

	headers += "\r\n";

	if (m_body)
	    headers += m_body;

	rc = m_socket->WriteAll(headers.c_str(), headers.length());
	if (rc != 0)
	{
	    TRACE << "Can't even write headers: " << rc << "\n";
	    return rc;
	}

//	TRACE << "Sent headers:\n" << headers;
	rc = m_socket->SetNonBlocking(true);

	util::PeekingLineReader lr(m_socket);
	http::Parser hp(&lr);

	if (rc != 0)
	{
	    TRACE << "Can't set non-blocking: " << rc << "\n";
	    return rc;
	}

	bool is_error = false;

	for (;;)
	{
	    unsigned int httpcode;

	    rc = hp.GetResponseLine(&httpcode, NULL);

//	    TRACE << "GetResponseLine returned " << rc << "\n";
	    
	    if (rc == 0)
	    {
		is_error = (httpcode != 206 && httpcode != 200);
		break;
	    }
	    if (rc != EWOULDBLOCK)
	    {
		TRACE << "GetLine failed " << rc << "\n";
		return rc;
	    }
	    if (rc == EWOULDBLOCK)
	    {
		rc = m_socket->WaitForRead(5000);
		if (rc != 0)
		{
		    TRACE << "Socket won't come ready " << rc << "\n";
		    return rc;
		}
	    }
	}
	
	m_socket->SetNonBlocking(false);

	bool got_range = false;
	unsigned long long clen = 0;

	for (;;)
	{
	    std::string key, value;
	    rc = hp.GetHeaderLine(&key, &value);
	    if (rc)
	    {
		TRACE << "GetHeaderLine says " << rc << ", bailing\n";
		return rc;
	    }

	    if (!strcasecmp(key.c_str(), "Content-Range"))
	    {
		unsigned long long rmin, rmax, elen = 0;

		/* HTTP/1.1 says "Content-Range: bytes X-Y/Z"
		 * but traditional Receiver servers send
		 * "Content-Range: bytes=X-Y"
		 */

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
			m_len = elen;
		    else
			m_len = rmax + 1;
		    
		    got_range = true;
		}
	    }
	    else if (!strcasecmp(key.c_str(), "Content-Length"))
	    {
		sscanf(value.c_str(), "%llu", &clen);
	    }
	    else if (key.empty())
		break;
	}

	if (!got_range)
	    m_len = clen;

	if (is_error)
	{
	    util::StreamPtr epage = CreatePartialStream(m_socket, clen);
	    StringStreamPtr ssp = StringStream::Create();
	    CopyStream(epage, ssp);
	    TRACE << "HTTP error page: " << ssp->str() << "\n";
	    return EINVAL;
	}

	m_need_fetch = false;

	m_last_pos = pos;
    }
	
    unsigned int rc = m_socket->Read(buffer, len, pread);
    if (!rc)
	m_last_pos += *pread;
    return rc;
}

unsigned Stream::WriteAt(const void*, pos64, size_t, size_t*)
{
    return EPERM;
}

SeekableStream::pos64 Stream::GetLength() { return m_len; }

unsigned Stream::SetLength(pos64) { return EPERM; }

} // namespace http

} // namespace util

#ifdef TEST

# include "http_server.h"
# include "poll.h"
# include "string_stream.h"
# include "worker_thread_pool.h"

class FetchTask: public util::Task
{
    std::string m_url;
    std::string m_contents;
    util::PollerInterface *m_waker;
    volatile bool m_done;

public:
    FetchTask(const std::string& url, util::PollerInterface *waker)
	: m_url(url), m_waker(waker), m_done(false) {}

    unsigned int Run()
    {
//	TRACE << "Fetcher running\n";
	util::http::StreamPtr hsp;
	unsigned int rc = util::http::Stream::Create(&hsp, m_url.c_str());
	assert(rc == 0);

	char buf[2048];
	size_t nread;
	rc = hsp->Read(buf, sizeof(buf), &nread);
	assert(rc == 0);
	m_contents = std::string(buf, buf+nread);
	m_waker->Wake();
//	TRACE << "Fetcher done " << rc << "\n";
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
    util::Poller poller;
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);

    util::http::Server ws(&poller, &wtp);

    unsigned rc = ws.Init();

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    assert(rc == 0);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    FetchTask *ft = new FetchTask(url, &poller);

    util::TaskPtr tp(ft);
    wtp.PushTask(tp);

    time_t start = time(NULL);
    time_t finish = start+5;
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "polling for " << (finish-now)*1000 << "ms\n";
	    poller.Poll((unsigned)(finish-now)*1000);
	}
    } while (now < finish && !ft->IsDone());

//    TRACE << "Got '" << ft->GetContents() << "' (len "
//	  << strlen(ft->GetContents().c_str()) << " sz "
//	  << ft->GetContents().length() << ")\n";
    assert(ft->GetContents() == "/zootle/wurdle.html");

    return 0;
}

#endif // TEST
