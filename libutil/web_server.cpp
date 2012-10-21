#include "config.h"
#include "web_server.h"
#include "libutil/async_write_buffer.h"
#include "libutil/socket.h"
#include "libutil/poll.h"
#include "libutil/trace.h"
#include "libutil/magic.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include "http_client.h"
#include "worker_thread.h"
#include "string_stream.h"
#include "partial_stream.h"
#include "line_reader.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

#undef IN
#undef OUT

namespace util {

class WebServer::Task: public util::Task
{
    WebServer *m_parent;
    StreamSocket m_socket;

public:
    Task(WebServer *parent, StreamSocket *server);

    // Being a Task
    void Run();
};

WebServer::Task::Task(WebServer *parent, StreamSocket *server)
    : m_parent(parent)
{
    unsigned int rc = server->Accept(&m_socket);
    if (rc != 0)
    {
	TRACE << "Accept failed " << rc << "\n";
	m_socket.Close();
    }
//    TRACE << "Accepted socket local ep " 
//	  << m_socket.GetLocalEndPoint().ToString() << "\n";
}

void WebServer::Task::Run()
{
    if (!m_socket.IsOpen())
	return;
//    TRACE << "In WSTR with live socket\n";

    StreamPtr stream = m_socket.CreateStream();

    util::LineReader lr(stream);

    unsigned int count = 0;

    for (;;)
    {
	++count;

	unsigned rc = m_socket.SetNonBlocking(true);
	if (rc != 0)
	{
	    TRACE << "Can't set non-blocking: " << rc << "\n";
	    return;
	}

	std::string line;

	for (;;)
	{
	    rc = lr.GetLine(&line);
//	    TRACE << "GetLine returned " << rc << "\n";
	    if (rc == 0)
		break;
	    if (rc != EWOULDBLOCK)
	    {
		if (rc != EIO)
		    TRACE << "GetLine failed " << rc << "\n";
		return;
	    }
	    if (rc == EWOULDBLOCK)
	    {
		rc = m_socket.WaitForRead(5000);
		if (rc != 0)
		{
		    TRACE << (void*)this << " unreadable " << rc
			  << " (request " << count << ")\n";
		    return;
		}
	    }
	}

	m_socket.SetNonBlocking(false);

//	TRACE << "Line   '" << line << "'\n";

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" \t");

	WebRequest rq;
	rq.local_ep = m_socket.GetLocalEndPoint();

	{
	    tokenizer bt(line, sep);
	    tokenizer::iterator bti = bt.begin();
	    tokenizer::iterator end = bt.end();

	    if (bti == end)
	    {
		TRACE << "Don't like request line '" << line << "'\n";
		return;
	    }
	    if (*bti != "GET")
	    {
		TRACE << "Don't like non-get line '" << line << "'\n";
		return;
	    }
	    ++bti;
	    if (bti == end)
	    {
		TRACE << "Don't like request line '" << line << "'\n";
		return;
	    }
	    rq.path = *bti;
	    ++bti;
	    if (bti == end)
	    {
		TRACE << "Don't like request line '" << line << "'\n";
		return;
	    }
//	    TRACE << "Looking for path " << path << " version " << *bti
//		  << "\n";
	}

	bool closing = false;
	bool do_range = false;
	unsigned long long range_min = 0;
	unsigned long long range_max = 0;
	std::string header;
	do {
	    rc = lr.GetLine(&header);
	    if (rc)
	    {
		TRACE << "Get headers failed " << rc << "\n";
		return;
	    }
//	    TRACE << "Header '" << header << "'\n";
	    
	    tokenizer bt(header, sep);
	    tokenizer::iterator bti = bt.begin();
	    tokenizer::iterator end = bt.end();
	    if (bti != end)
	    {
		if (!strcasecmp(bti->c_str(), "Connection:"))
		{
		    ++bti;
		    if (bti != end)
		    {
			if (!strcasecmp(bti->c_str(), "Close"))
			    closing = true;
		    }
		}
		else if (!strcasecmp(bti->c_str(), "Range:"))
		{
		    ++bti;
		    if (bti != end)
		    {
			if (sscanf(bti->c_str(), "bytes=%llu-%llu", &range_min,
				   &range_max) == 2)
			{
			    do_range = true;
			    ++range_max; // HTTP is inc/inc; we want inc/exc
			}
			else if (sscanf(bti->c_str(), "bytes=%llu-", &range_min)
				 == 1)
			{
			    do_range = true;
			    range_max = (unsigned long long)-1; 
			    // Will be clipped against len later
			}
			else
			    TRACE << "Don't like range request '" << *bti
				  << "'\n";
		    }
		}
		else if (!strcasecmp(bti->c_str(), "Cache-Control:"))
		    rq.refresh = true;
	    }
	} while (header != "");

	WebResponse rs;
	m_parent->StreamForPath(&rq, &rs);

	std::string headers;
	if (!rs.ssp)
	{
	    headers = "HTTP/1.1 404 Not Found\r\n";
	    StringStreamPtr ssp = StringStream::Create();
	    ssp->str() = "<i>404, dude, it's just not there</i>";
	    rs.ssp = ssp;
	}
	else if (do_range)
	    headers = "HTTP/1.1 206 OK But A Bit Partial\r\n";
	else
	    headers = "HTTP/1.1 200 OK\r\n";

	headers += "Server: " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n";
	headers += "Accept-Ranges: bytes\r\n";
	headers += "Content-Type: ";
	    
	if (rs.content_type)
	    headers += rs.content_type;
	else
	    headers += "text/html";

	headers += "\r\n";

	unsigned long long len;

	if (rs.ssp)
	{
	    len = rs.ssp->GetLength();

	    if (do_range)
	    {
		if (range_min > len)
		    range_min = len;
		if (range_max > len)
		    range_max = len;

		/* This form of the Content-Range header is completely
		 * bogus (the '=' should be whitespace), but is what
		 * Receivers expect.
		 */
		headers += (boost::format("Content-Range: bytes=%llu-%llu\r\n")
			    % range_min
			    % (range_max-1)).str();
//		headers += (boost::format("Content-Range: bytes %llu-%llu/%llu\r\n")
//			    % range_min
//			    % (range_max-1)
//			    % len).str();

		if (range_min > 0 || range_max != len)
		    rs.ssp = util::PartialStream::Create(rs.ssp, range_min,
						      range_max);

		len = range_max - range_min;
	    }
	}
	else
	    len = 0;

	headers += (boost::format("Content-Length: %llu\r\n") % len).str();
	headers += "\r\n";

	m_socket.SetCork(true);

//	TRACE << "Response headers:\n" << headers;

	rc = stream->WriteAll(headers.c_str(), headers.length());

	m_socket.SetCork(false);

	if (rc != 0)
	{
	    TRACE << "Writing headers failed " << rc << "\n";
	    return;
	}

//	TRACE << (void*)this << " wrote headers\n";

	if (rs.ssp)
	{
	    if (0)//len > 128*1024)
	    {
		StreamPtr awb;
		rc = AsyncWriteBuffer::Create(stream,
					      m_parent->GetAsyncQueue(),
					      &awb);
		if (rc == 0)
		    rc = util::CopyStream(rs.ssp, awb);
		else
		    rc = util::CopyStream(rs.ssp, stream);
	    }
	    else
		rc = util::CopyStream(rs.ssp, stream);

	    if (rc != 0)
	    {
		TRACE << "Writing stream failed " << rc << "\n";
		return;
	    }
	}

//	TRACE << (void*)this << " wrote stream\n";

	m_socket.SetCork(false);

	if (closing)
	    return;

//	TRACE << "Going round again on same connection\n";
    }
}


        /* WebServer itself */


WebServer::WebServer()
    : m_port(0),
      m_threads(32),
      m_async_threads(8)
{
}

WebServer::~WebServer()
{
}

unsigned WebServer::Init(util::PollerInterface *poller, const char *,
			 unsigned short port)
{
    IPEndPoint ep = { IPAddress::ANY, port };
    unsigned int rc = m_server_socket.Bind(ep);
    if (rc != 0)
    {
	TRACE << "WebServer bind failed " << rc << "\n";
	return rc;
    }

    ep = m_server_socket.GetLocalEndPoint();

    m_server_socket.SetNonBlocking(true);
    m_server_socket.Listen();
    poller->AddHandle(m_server_socket.GetPollHandle(PollerInterface::IN),
		      this, PollerInterface::IN);

    TRACE << "WebServer got port " << ep.port << "\n";
    m_port = ep.port;
    return 0;
}

unsigned WebServer::OnActivity()
{
//    TRACE << "Got activity, trying to accept\n";
    m_threads.GetTaskQueue()->PushTask(
	TaskPtr(new WebServer::Task(this, &m_server_socket)));
    return 0;
}

TaskQueue *WebServer::GetAsyncQueue()
{
    return m_async_threads.GetTaskQueue();
}

unsigned short WebServer::GetPort()
{
    return m_port;
}

void WebServer::StreamForPath(const WebRequest *rq, WebResponse *rs)
{
    if (rq->path == m_cached_url)
    {
	if (rq->refresh)
	{
	    m_cached_url.clear();
	    m_cache = NULL;
	}
	else
	{
	    if (m_cache)
		m_cache->Seek(0);
	    rs->content_type = m_cached_type;
	    rs->ssp = m_cache;
	    return;
	}
    }

    for (list_t::iterator i = m_content.begin(); 
	 i != m_content.end();
	 ++i)
    {
	bool taken = (*i)->StreamForPath(rq, rs);
	if (taken)
	{
	    m_cached_url = rq->path;
	    m_cache = rs->ssp;
	    m_cached_type = rs->content_type;
	    return;
	}
    }
}

void WebServer::AddContentFactory(const std::string&, ContentFactory *cf)
{
    m_content.push_back(cf);
}


        /* FileContentFactory */


bool FileContentFactory::StreamForPath(const WebRequest *rq,
				       WebResponse *rs)
{
    TRACE << "Request for page '" << rq->path << "'\n";
    if (strncmp(rq->path.c_str(), m_page_root.c_str(), m_page_root.length()))
    {
	TRACE << "Not in my root '" << m_page_root << "'\n";
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

    TRACE << "Path '" << rq->path << "' is file '" << path2 << "'\n";
    return true;
}

} // namespace util

#ifdef TEST

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
	util::HttpClient hc(m_url);
	unsigned int rc = hc.FetchToString(&m_contents);
	m_waker->Wake();
//	TRACE << "Fetcher done " << rc << "\n";
	if (!rc)
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

#endif
