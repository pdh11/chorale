#include "config.h"
#include "http_stream.h"
#include "http_client.h"
#include "trace.h"
#include "line_reader.h"
#include <errno.h>
#include <string.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

namespace util {

HTTPStream::HTTPStream(const IPEndPoint& ipe, const std::string& path)
    : m_ipe(ipe),
      m_path(path),
      m_len(0),
      m_need_fetch(true),
      m_stm(m_socket.CreateStream())
{
}

HTTPStream::~HTTPStream()
{
}

unsigned HTTPStream::Create(HTTPStreamPtr *result, const char *url)
{
    std::string host;
    IPEndPoint ipe;
    std::string path;
    ParseURL(url, &host, &path);
    ParseHost(host, 80, &host, &ipe.port);
    ipe.addr = IPAddress::Resolve(host.c_str());
    if (ipe.addr.addr == 0)
	return ENOENT;

    *result = HTTPStreamPtr(new HTTPStream(ipe, path));

    return 0;
}

unsigned HTTPStream::ReadAt(void *buffer, pos64 pos, size_t len,
			    size_t *pread)
{
    /** @todo Isn't thread-safe like the other ReadAt's. Add a mutex. */

    if (!m_need_fetch && pos != Tell())
    {
	m_socket.Close();
	m_socket.Open();
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
	m_socket.SetNonBlocking(false);

	unsigned int rc = m_socket.Connect(m_ipe);
	if (rc != 0)
	    return rc;

	std::string headers;
	headers = "GET " + m_path + " HTTP/1.1\r\n";

	headers += (boost::format("Range: bytes=%llu-\r\n")
		    % (unsigned long long)pos).str();
	headers += "User-Agent: " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";
	headers += "\r\n";

	rc = m_stm->WriteAll(headers.c_str(), headers.length());
	if (rc != 0)
	{
	    TRACE << "Can't even write headers: " << rc << "\n";
	    return rc;
	}

//	TRACE << "Sent headers:\n" << headers;

	util::LineReader lr(m_stm);

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" \t");

	rc = m_socket.SetNonBlocking(true);
	if (rc != 0)
	{
	    TRACE << "Can't set non-blocking: " << rc << "\n";
	    return rc;
	}

	std::string line;

	for (;;)
	{
	    rc = lr.GetLine(&line);
//	TRACE << "GetLine returned " << rc << "\n";
	    if (rc == 0)
		break;
	    if (rc != EWOULDBLOCK)
	    {
		TRACE << "GetLine failed " << rc << "\n";
		return rc;
	    }
	    if (rc == EWOULDBLOCK)
	    {
		rc = m_socket.WaitForRead(5000);
		if (rc != 0)
		{
		    TRACE << "Socket won't come ready " << rc << "\n";
		    return rc;
		}
	    }
	}
	
	m_socket.SetNonBlocking(false);

	{
	    tokenizer bt(line, sep);
	    tokenizer::iterator bti = bt.begin();
	    tokenizer::iterator end = bt.end();
	    
	    if (bti == end)
	    {
		TRACE << "Don't like response line '" << line << "'\n";
		return ENOENT;
	    }
	    if (*bti != "HTTP/1.0" && *bti != "HTTP/1.1")
	    {
		TRACE << "Don't like HTTP version '" << line << "'\n";
		return ENOENT;
	    }
	    ++bti;
	    if (bti == end)
	    {
		TRACE << "Don't like request line '" << line << "'\n";
		return ENOENT;
	    }
	    unsigned int httpcode = atoi(bti->c_str());
	    ++bti;
	    if (bti == end)
	    {
		TRACE << "Don't like request line '" << line << "'\n";
		return ENOENT;
	    }
	    
	    if (httpcode != 206 && httpcode != 200)
	    {
		TRACE << "Don't like HTTP response " << httpcode << " ('"
		      << line << "')\n";
		return ENOENT;
	    }
	}

	do {
	    rc = lr.GetLine(&line);
//	    TRACE << "Header " << line << "\n";

	    tokenizer bt(line, sep);
	    tokenizer::iterator bti = bt.begin();
	    tokenizer::iterator end = bt.end();
	    if (bti != end)
	    {
		if (!strcasecmp(bti->c_str(), "Content-Range:"))
		{
		    ++bti;

		    if (bti != end && !strcasecmp(bti->c_str(), "bytes"))
			++bti;

		    if (bti != end)
		    {
			unsigned long long rmin, rmax, elen = 0;

			/* HTTP/1.1 says "Content-Range: bytes X-Y/Z"
			 * but traditional Receiver servers send
			 * "Content-Range: bytes=X-Y"
			 */

			if (sscanf(bti->c_str(), "%llu-%llu/%llu",
				   &rmin, &rmax, &elen) == 3
			    || sscanf(bti->c_str(), "bytes=%llu-%llu/%llu",
				      &rmin, &rmax, &elen) == 3
			    || sscanf(bti->c_str(), "%llu-%llu",
				      &rmin, &rmax) == 2
			    || sscanf(bti->c_str(), "bytes=%llu-%llu",
				      &rmin, &rmax) == 2)
			{
			    if (elen)
				m_len = elen;
			    else
				m_len = rmax + 1;
			}
		    }
		}
	    }
	} while (line != "");

	size_t nleft;
	const char *leftovers = lr.GetLeftovers(&nleft);

	m_need_fetch = false;

	if (nleft)
	{
	    memcpy(buffer, leftovers, nleft);
	    *pread = nleft;
	    return 0;
	}
    }
	
    unsigned int rc = m_stm->Read(buffer, len, pread);
    return rc;
}

unsigned HTTPStream::WriteAt(const void*, pos64, size_t, size_t*)
{
    return EPERM;
}

SeekableStream::pos64 HTTPStream::GetLength() { return m_len; }

unsigned HTTPStream::SetLength(pos64) { return EPERM; }

} // namespace util

#ifdef TEST

# include "web_server.h"
# include "poll.h"
# include "string_stream.h"
# include "worker_thread.h"

class FetchTask: public util::Task
{
    std::string m_url;
    std::string m_contents;
    util::PollWaker *m_waker;
    volatile bool m_done;

public:
    FetchTask(const std::string& url, util::PollWaker *waker)
	: m_url(url), m_waker(waker), m_done(false) {}

    void Run()
    {
//	TRACE << "Fetcher running\n";
	util::HTTPStreamPtr hsp;
	unsigned int rc = util::HTTPStream::Create(&hsp, m_url.c_str());
	assert(rc == 0);

	char buf[2048];
	size_t nread;
	rc = hsp->Read(buf, sizeof(buf), &nread);
	assert(rc == 0);
	m_contents = std::string(buf, buf+nread);
	m_waker->Wake();
//	TRACE << "Fetcher done " << rc << "\n";
	m_done = true;
    }

    const std::string& GetContents() const { return m_contents; }

    bool IsDone() const { return m_done; }
};

class EchoContentFactory: public util::ContentFactory
{
public:
    bool StreamForPath(const util::WebRequest *rq, util::WebResponse *rs)
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

    util::WebServer ws;

    unsigned rc = ws.Init(&poller, "/", 0);

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    assert(rc == 0);

    util::PollWaker waker(&poller);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    FetchTask *ft = new FetchTask(url, &waker);

    util::TaskQueue queue;
    util::WorkerThread wt(&queue);

    util::TaskPtr tp(ft);
    queue.PushTask(tp);

    time_t start = time(NULL);
    time_t finish = start+2;
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

    queue.PushTask(util::TaskPtr(NULL));

    return 0;
}

#endif // TEST
