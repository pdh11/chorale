#include "http_fetcher.h"
#include "http_client.h"
#include "trace.h"
#include "errors.h"
#include "scheduler.h"
#include "file.h"
#include "counted_pointer.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

namespace util {

namespace http {

class Fetcher::Task: public Connection
{
    std::map<std::string, std::string> *m_headers;
    unsigned int m_error_code;
    bool m_done;
    std::string m_response;
    IPEndPoint m_local_endpoint;

    // Being a util::http::Connection
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    void OnHeader(const std::string& /*key*/, const std::string& /*value*/);
    void OnEndPoint(const util::IPEndPoint& endpoint);
    void OnDone(unsigned int error_code);

public:
    explicit Task(std::map<std::string, std::string> *headers);
    ~Task() {}

    bool IsDone() const { return m_done; }
    IPEndPoint GetLocalEndPoint() const { return m_local_endpoint; }
    unsigned int GetErrorCode() const { return m_error_code; }
    const std::string& GetResponse() const { return m_response; }
};

Fetcher::Task::Task(std::map<std::string, std::string> *headers)
    : m_headers(headers),
      m_error_code(0),
      m_done(false)
{
}

void Fetcher::Task::OnHeader(const std::string& key, const std::string& value)
{
//    TRACE << "hf" << this << ": " << key << ": " << value << "\n";
    (*m_headers)[key] = value;
}

void Fetcher::Task::OnEndPoint(const util::IPEndPoint& endpoint)
{
    m_local_endpoint = endpoint;
}

unsigned Fetcher::Task::Write(const void *buffer, size_t len, size_t *pwrote)
{
    m_response.append((const char*)buffer, len);
    *pwrote = len;
    return 0;
}

void Fetcher::Task::OnDone(unsigned int error_code)
{
    if (!m_error_code)
	m_error_code = error_code;
    m_done = true;
}


Fetcher::Fetcher(Client *client, 
		 const std::string& url, const char *extra_headers,
		 const char *body,
		 const char *verb)
    : m_client(client),
      m_url(url),
      m_extra_headers(extra_headers),
      m_body(body),
      m_verb(verb)
{
}

Fetcher::~Fetcher()
{
}

unsigned int Fetcher::FetchToString(std::string *presult)
{
    // Use asynchronous API synchronously by having our own Scheduler
    BackgroundScheduler scheduler;

    CountedPointer<Task> ptr(new Task(&m_headers));

    unsigned int rc = m_client->Connect(&scheduler, ptr, m_url,
					m_extra_headers ? m_extra_headers : "",
					m_body ? m_body : "", m_verb);
    if (rc)
	return rc;

    time_t start = time(NULL);
    time_t finish = start+10; // Give it ten seconds
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "hf" << this << ": hc" << m_connection << ": polling\n";
	    rc = scheduler.Poll((unsigned)(finish-now)*1000);
	    if (rc)
		return rc;
	}
    } while (now < finish && !ptr->IsDone() && !ptr->GetErrorCode());
    
    if (!ptr->IsDone())
    {
	rc = ptr->GetErrorCode();
	if (!rc)
	{
//	    TRACE << "hf" << this << ": hc" << m_connection << ": timed out\n";
	    rc = ETIMEDOUT;
	}
    }
    else
    {
	m_local_endpoint = ptr->GetLocalEndPoint();
    }

    if (!rc)
	*presult = ptr->GetResponse();
    return rc;
}


        /* Utility functions */


void ParseURL(const std::string& url,
	      std::string *hostpart,
	      std::string *pathpart)
{
    const char *ptr = url.c_str();
    const char *colon = strchr(ptr, ':');
    const char *slash = strchr(ptr, '/');
    if (colon && slash && slash > colon && slash[1] == '/')
    {
	slash = strchr(slash+2, '/');
	if (slash)
	{
	    *pathpart = slash;
	    if (hostpart)
		*hostpart = std::string(ptr, (size_t)(slash-ptr));
	}
	else
	{
	    *pathpart = "/";
	    if (hostpart)
		*hostpart = url;
	}
	return;
    }
    if (hostpart)
	*hostpart = std::string();
    *pathpart = url;
}

void ParseHost(const std::string& hostpart, unsigned short default_port,
	       std::string *hostname, unsigned short *port)
{
    const char *ptr = hostpart.c_str();
    const char *colon = strchr(ptr, ':');
    const char *slash = strchr(ptr, '/');
    if (colon && slash && slash > colon && slash[1] == '/')
    {
	ptr = slash+2;
	colon = strchr(ptr, ':');
	if (colon)
	{
	    *hostname = std::string(ptr, colon);
	    *port = (unsigned short)atoi(colon+1);
	    return;
	}
    }

    *hostname = ptr;
    *port = default_port;
}

std::string ResolveURL(const std::string& base,
		       const std::string& link)
{
    std::string linkhost, linkpath;
    ParseURL(link, &linkhost, &linkpath);
    if (!linkhost.empty())
	return link;
    std::string basehost, basepath;
    ParseURL(base, &basehost, &basepath);
    return basehost + util::MakeAbsolutePath(basepath, linkpath);
}

bool IsHttpURL(const char *url)
{
    return !strncmp(url, "http://", 7);
}

} // namespace http

} // namespace util

#ifdef TEST

# include "scheduler.h"
# include "http_server.h"
# include "worker_thread_pool.h"
# include "string_stream.h"
# include <boost/format.hpp>

static struct {
    const char *base;
    const char *link;
    const char *expect;
} tests[] = {
    { "http://foo.bar/foo", "frink",        "http://foo.bar/frink" },
    { "http://foo.bar/foo/", "frink",       "http://foo.bar/foo/frink" },
    { "http://foo.bar/foo/", "/frink",      "http://foo.bar/frink" },
    { "http://foo.bar",      "wurdle",      "http://foo.bar/wurdle" },
    { "http://foo.bar",      "/wurdle",     "http://foo.bar/wurdle" },
    { "http://foo.bar:2888", "wurdle",      "http://foo.bar:2888/wurdle" },
    { "http://foo.bar:2888","/wurdle",      "http://foo.bar:2888/wurdle" }
};

#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

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

int main()
{
    for (unsigned int i=0; i<COUNTOF(tests); ++i)
    {
	std::string result = util::http::ResolveURL(tests[i].base, 
						    tests[i].link);
	if (result != tests[i].expect)
	{
	    TRACE << "Resolve(" << tests[i].base << ", " << tests[i].link
		  << ") = '" << result << "' should be '" << tests[i].expect
		  << "')\n";
	    return 1;
	}
    }
    
    std::string url = "http://foo.bar:2888/wurdle";
    std::string hostpart, host, path;
    unsigned short port;

    util::http::ParseURL("file:///usr/src/chorale", NULL, &path);
    assert(path == "/usr/src/chorale");

    util::http::ParseURL(url, &hostpart, &path);
//    TRACE << "hp=" << hostpart << ", path=" << path << "\n";
    assert(hostpart == "http://foo.bar:2888");
    assert(path == "/wurdle");
    
    util::http::ParseHost(hostpart, 80, &host, &port);
//    TRACE << "host=" << host << ", port=" << port << "\n";
    assert(host == "foo.bar");
    assert(port == 2888);

    util::WorkerThreadPool server_threads(util::WorkerThreadPool::NORMAL, 4);
    util::BackgroundScheduler poller;
    util::http::Server ws(&poller, &server_threads);
    server_threads.PushTask(util::SchedulerTask::Create(&poller));

    unsigned rc = ws.Init();
    assert(rc == 0);

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    std::string url2 = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::http::Client hc;
    util::http::Fetcher hf(&hc, url2, "Range: bytes=0-\r\n");
    std::string contents;
    rc = hf.FetchToString(&contents);
    assert(rc == 0);
    assert(contents == "/zootle/wurdle.html");

    return 0;
}

#endif
