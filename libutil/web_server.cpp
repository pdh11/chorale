#include "config.h"
#include "web_server.h"
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

    for (;;)
    {
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
		TRACE << "GetLine failed " << rc << "\n";
		return;
	    }
	    if (rc == EWOULDBLOCK)
	    {
		rc = m_socket.WaitForRead(5000);
		if (rc != 0)
		{
//		    TRACE << "Socket won't come ready " << rc << "\n";
		    return;
		}
	    }
	}

	m_socket.SetNonBlocking(false);

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" \t");

	std::string path;
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
	    path = *bti;
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
	unsigned int range_min = 0;
	unsigned int range_max = 0;
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
			if (sscanf(bti->c_str(), "bytes=%u-%u", &range_min,
				   &range_max) == 2)
			{
			    do_range = true;
			    ++range_max; // HTTP is inc/inc; we want inc/exc
			}
			else
			    TRACE << "Don't like range request '" << *bti
				  << "'\n";
		    }
		}
	    }
	} while (header != "");

	SeekableStreamPtr ssp = m_parent->StreamForPath(path.c_str());

	std::string headers;
	if (!ssp)
	    headers = "HTTP/1.1 404 Not Found\r\n";
	else if (do_range)
	    headers = "HTTP/1.1 206 OK But A Bit Partial\r\n";
	else
	    headers = "HTTP/1.1 200 OK\r\n";

	headers += "Server: " PACKAGE_STRING "\r\n";

	if (ssp)
	{
	    unsigned int len = ssp->GetLength();

	    if (do_range)
	    {
		if (range_min > len)
		    range_min = len;
		if (range_max > len)
		    range_max = len;

		headers += (boost::format("Content-Range: %u-%u/%u\r\n")
			    % range_min
			    % (range_max-1)
			    % len).str();

		len = range_max - range_min;

		ssp = util::PartialStream::Create(ssp, range_min, range_max);
	    }
	    headers += (boost::format("Content-Length: %u\r\n") % len).str();
	}

	headers += "\r\n";

	m_socket.SetCork(true);

	rc = stream->WriteAll(headers.c_str(), headers.length());

	if (rc != 0)
	{
	    TRACE << "Writing headers failed " << rc << "\n";
	    return;
	}
	if (ssp)
	{
	    rc = stream->Copy(ssp);

	    if (rc != 0)
	    {
		TRACE << "Writing stream failed " << rc << "\n";
		return;
	    }
	}

	m_socket.SetCork(false);

	if (closing)
	    return;

//	TRACE << "Going round again on same connection\n";
    }
}


        /* WebServer itself */


WebServer::WebServer()
    : m_threads(12)
{
}

WebServer::~WebServer()
{
}

unsigned WebServer::Init(util::PollerInterface *poller, const char *,
			 unsigned short port)
{
    IPEndPoint ep = { IPAddress::ANY, port };
    int rc = m_server_socket.Bind(ep);
    if (rc < 0)
    {
	TRACE << "WebServer bind failed " << errno << "\n";
	return errno;
    }

    ep = m_server_socket.GetLocalEndPoint();

    m_server_socket.SetNonBlocking(true);
    m_server_socket.Listen();
    poller->AddHandle(m_server_socket.GetPollHandle(), this);

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

unsigned short WebServer::GetPort()
{
    return m_port;
}

SeekableStreamPtr WebServer::StreamForPath(const char *path)
{
    if (path == m_cached_url)
    {
	m_cache->Seek(0);
	return m_cache;
    }

    SeekableStreamPtr s;

    for (list_t::iterator i = m_content.begin(); 
	 i != m_content.end();
	 ++i)
    {
	s = (*i)->StreamForPath(path);
	if (s)
	{
	    m_cached_url = path;
	    m_cache = s;
	    return s;
	}
    }
    return s;
}

void WebServer::AddContentFactory(const char *,
				  ContentFactory *cf)
{
    m_content.push_back(cf);
}


        /* FileContentFactory */


SeekableStreamPtr FileContentFactory::StreamForPath(const char *path)
{
    TRACE << "Request for page '" << path << "'\n";
    if (strncmp(path, m_page_root.c_str(), m_page_root.length()))
    {
	TRACE << "Not in my root '" << m_page_root << "'\n";
	return SeekableStreamPtr();
    }

    path += m_page_root.length();

    std::string path2 = m_file_root + path;
    path2 = util::Canonicalise(path2);

    // Make sure it's still under the right root (no "/../" attacks)
    if (strncmp(path2.c_str(), m_file_root.c_str(), m_file_root.length()))
    {
	TRACE << "Resolved file '" << path2 << "' not in my root '" 
	      << m_file_root << "'\n";
	return SeekableStreamPtr();
    }

    struct stat st;
    if (stat(path2.c_str(), &st) < 0)
    {
	TRACE << "Resolved file '" << path2 << "' not found\n";
	return SeekableStreamPtr(); // Not found
    }

    if (!S_ISREG(st.st_mode))
    {
	TRACE << "Resolved file '" << path2 << "' not a file\n";
	return SeekableStreamPtr(); // Not a regular file
    }

    FileStreamPtr fsp;
    unsigned int rc = FileStream::Create(path2.c_str(), O_RDONLY, &fsp);
    if (rc != 0)
    {
	TRACE << "Resolved file '" << path2 << "' won't open " << errno 
	      << "\n";
	return SeekableStreamPtr(); // Won't open
    }

    TRACE << "Path '" << path << "' is file '" << path2 << "'\n";

    return fsp;
}

};

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
	TRACE << "Fetcher running\n";
	util::HttpClient hc(m_url);
	unsigned int rc = hc.FetchToString(&m_contents);
	m_waker->Wake();
	TRACE << "Fetcher done " << rc << "\n";
	//m_done = true;
    }

    const std::string& GetContents() const { return m_contents; }

    bool IsDone() const { return m_done; }
};

class EchoContentFactory: public util::ContentFactory
{
public:
    util::SeekableStreamPtr StreamForPath(const char *path)
    {
	util::StringStreamPtr ssp = util::StringStream::Create();
	ssp->str() = path;
	return ssp;
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
	    TRACE << "polling for " << (finish-now)*1000 << "ms\n";
	    poller.Poll((finish-now)*1000);
	}
    } while (now < finish && !ft->IsDone());

    TRACE << "Got '" << ft->GetContents() << "' (len "
	  << strlen(ft->GetContents().c_str()) << " sz "
	  << ft->GetContents().length() << ")\n";
    assert(ft->GetContents() == "/zootle/wurdle.html");

    queue.PushTask(util::TaskPtr(NULL));

    return 0;
}

#endif
