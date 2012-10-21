#include "http_stream.h"
#include "config.h"
#include "http_client.h"
#include "http_fetcher.h"
#include "http_parser.h"
#include "partial_stream.h"
#include "string_stream.h"
#include "trace.h"
#include "errors.h"
#include "scanf64.h"
#include "peeking_line_reader.h"
#include <errno.h>
#include <string.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

LOG_DECL(HTTP);

namespace util {

namespace http {

Stream::Stream(Client *client, const IPEndPoint& ipe, const std::string& host,
	       const std::string& path)
    : m_client(client),
      m_ipe(ipe),
      m_host(host),
      m_path(path),
      m_socket(StreamSocket::Create()),
      m_len(0),
      m_need_fetch(true),
      m_last_pos(0)
{
}

Stream::~Stream()
{
    LOG(HTTP) << "~hs" << this << "\n";
}

unsigned Stream::Create(StreamPtr *result, Client *client, const char *url)
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

    Stream *stm = new Stream(client, ipe, host, path);

    LOG(HTTP) << "hs" << stm << ": " << url << "\n";

    *result = StreamPtr(stm);

    return 0;
}

unsigned Stream::ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread)
{
    /** @todo Isn't thread-safe like the other ReadAt's. Add a mutex. */

    LOG(HTTP) << "hs" << this << ".ReadAt(" << pos << ",+" << len << ")\n";

    if (!m_need_fetch && pos != m_last_pos)
    {
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

	LOG(HTTP) << "hs" << this << ": synchronous connect\n";

	unsigned int rc = m_socket->Connect(m_ipe);
	if (rc != 0)
	{
	    LOG(HTTP) << "hs" << this << " can't connect: " << rc << "\n";
	    return rc;
	}

	std::string headers = "GET";

	headers += " " + m_path + " HTTP/1.1\r\n";

	headers += "Host: " + m_host + "\r\n";

	if (pos)
	{
	    if (m_len)
		headers += (boost::format("Range: bytes=%llu-%llu\r\n")
			    % (unsigned long long)pos
			    % (unsigned long long)(m_len-1) ).str();
	    else
	    {
		/* This is a bit nasty. Some "traditional" Receiver
		 * servers (in particular, Jupiter) don't like
		 * one-ended ranges. All such servers are on the
		 * "traditional" Receiver port of 12078; all such
		 * servers don't deal with >4GB files anyway. They do,
		 * however, deal with clipping large ranges to the
		 * actual size.
		 */
		if (m_ipe.port == 12078)
		    headers += (boost::format(
				    "Range: bytes=%llu-4294967295\r\n")
				% (unsigned long long)pos).str();
		else
		    headers += (boost::format(
				    "Range: bytes=%llu-\r\n")
				% (unsigned long long)pos).str();
	    }
	}

	headers += "User-Agent: " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";
	headers += "\r\n";

	rc = m_socket->WriteAll(headers.c_str(), headers.length());
	if (rc != 0)
	{
	    TRACE << "Can't even write headers: " << rc << "\n";
	    return rc;
	}

	LOG(HTTP) << "hs" << this << " sent headers:\n" << headers;

	rc = m_socket->SetNonBlocking(true);

	if (rc != 0)
	{
	    TRACE << "Can't set non-blocking: " << rc << "\n";
	    return rc;
	}

	util::PeekingLineReader lr(m_socket);
	http::Parser hp(&lr);

	bool is_error = false;

	for (;;)
	{
	    unsigned int httpcode;

	    rc = hp.GetResponseLine(&httpcode, NULL);

	    LOG(HTTP) << "GetResponseLine returned " << rc << "\n";
	    
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
		    TRACE << "hs" << this << " socket won't come ready "
			  << rc << "\n";
		    return rc;
		}
	    }
	}
	
	m_socket->SetNonBlocking(false);

	bool got_range = false;
	uint64_t clen = 0;

	for (;;)
	{
	    std::string key, value;

	    rc = hp.GetHeaderLine(&key, &value);

	    // Note that PeekingLineReader uses ReadPeek which doesn't use
	    // the internal timeout.
	    if (rc == EWOULDBLOCK)
	    {
		rc = m_socket->WaitForRead(5000);
		if (rc != 0)
		{
		    TRACE << "hs" << this
			  << " socket won't come ready in headers "
			  << rc << "\n";
		    return rc;
		}
		continue;
	    }

	    if (rc)
	    {
		TRACE << "GetHeaderLine says " << rc << ", bailing\n";
		return rc;
	    }

	    if (key.empty())
		break;

	    LOG(HTTP) << "hs" << this << " " << key << ": " << value << "\n";

	    if (!strcasecmp(key.c_str(), "Content-Range"))
	    {
		uint64_t rmin, rmax, elen = 0;

		/* HTTP/1.1 says "Content-Range: bytes X-Y/Z"
		 * but traditional Receiver servers send
		 * "Content-Range: bytes=X-Y"
		 */
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
			m_len = elen;
		    else
			m_len = rmax + 1;
		    
		    got_range = true;
		}
	    }
	    else if (!strcasecmp(key.c_str(), "Content-Length"))
	    {
		util::Scanf64(value.c_str(), "%llu", &clen);
	    }
	}

	if (!got_range)
	    m_len = clen;

	if (is_error)
	{
	    util::StreamPtr epage = CreatePartialStream(m_socket, clen);
	    StringStream ssp;	    
	    CopyStream(epage.get(), &ssp);
	    TRACE << "HTTP error page: " << ssp.str() << "\n";
	    return EINVAL;
	}

	m_need_fetch = false;

	m_last_pos = pos;
    }
	
    unsigned int rc = m_socket->Read(buffer, len, pread);

    if (!rc)
    {
	m_last_pos += *pread;
	LOG(HTTP) << "hs" << this << " socket read " << *pread << " bytes\n";
    }
    else
    {
	LOG(HTTP) << "hs" << this << " socket read error " << rc << "\n";
    }

    return rc;
}

unsigned Stream::WriteAt(const void*, pos64, size_t, size_t*)
{
    return EPERM;
}

SeekableStream::pos64 Stream::GetLength() { return m_len; }

unsigned Stream::SetLength(pos64 len) 
{
    if (m_len && m_len != len)
	return EPERM;
    m_len = len;
    return 0;
}

} // namespace http

} // namespace util

#ifdef TEST

# include "http_server.h"
# include "scheduler.h"
# include "string_stream.h"
# include "worker_thread_pool.h"

class FetchTask: public util::Task
{
    std::string m_url;
    util::http::Client *m_client;
    std::string m_contents;
    util::Scheduler *m_waker;
    volatile bool m_done;

public:
    FetchTask(const std::string& url, util::http::Client *client, 
	      util::Scheduler *waker)
	: m_url(url), m_client(client), m_waker(waker), m_done(false) {}

    unsigned int Run()
    {
//	TRACE << "Fetcher running\n";
	util::http::StreamPtr hsp;
	unsigned int rc = util::http::Stream::Create(&hsp, m_client,
						     m_url.c_str());
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
    util::BackgroundScheduler poller;
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);

    util::http::Client wc;
    util::http::Server ws(&poller, &wtp);

    unsigned rc = ws.Init();

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    assert(rc == 0);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::CountedPointer<FetchTask> ft(new FetchTask(url, &wc, &poller));

    wtp.PushTask(util::Bind<FetchTask,&FetchTask::Run>(ft));

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
