#include "http_fetcher.h"
#include "http_client.h"
#include "trace.h"
#include "errors.h"
#include "poll.h"
#include "file.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

namespace util {

namespace http {

class Fetcher::Impl: public Connection::Observer
{
    std::map<std::string, std::string> *m_headers;
    unsigned int m_error_code;
    // Use asynchronous API synchronously by having our own Poller
    Poller m_poller;
    ConnectionPtr m_connection;
    bool m_done;
    std::string m_response;
    IPEndPoint m_local_endpoint;

    // Being a util::http::Connection::Observer
    unsigned OnHttpHeader(const std::string& key, const std::string& value);
    unsigned OnHttpData();
    void OnHttpDone(unsigned int error_code);

public:
    Impl(std::map<std::string, std::string> *headers,
	 Client *client, 
	 const std::string& url, 
	 const char *extra_headers,
	 const char *body,
	 const char *verb);
    ~Impl() {}

    unsigned int FetchToString(std::string *presult);
    IPEndPoint GetLocalEndPoint() { return m_local_endpoint; }
};

Fetcher::Impl::Impl(std::map<std::string, std::string> *headers,
		    Client *client, 
		    const std::string& url, 
		    const char *extra_headers,
		    const char *body,
		    const char *verb)
    : m_headers(headers),
      m_error_code(0),
      m_done(false)
{
    m_connection = client->Connect(&m_poller, this, url,
				   extra_headers ? extra_headers : "",
				   body ? body : "",
				   verb);
    m_error_code = m_connection->Init();
}

unsigned int Fetcher::Impl::FetchToString(std::string *presult)
{
    *presult = std::string();

    if (m_error_code)
	return m_error_code;

    time_t start = time(NULL);
    time_t finish = start+10; // Give it ten seconds
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "hf" << this << ": hc" << m_connection << ": polling\n";
	    unsigned int rc = m_poller.Poll((unsigned)(finish-now)*1000);
	    if (rc)
		return rc;
	}
    } while (now < finish && !m_done && !m_error_code);

    if (!m_done)
    {
//	TRACE << "hf" << this << ": hc" << m_connection << ": timed out\n";
    }
    else
    {
//	TRACE << "hf" << this << ": hc" << m_connection << ": done, error code is " << m_error_code
//	      << " response is '" << m_response << "'\n";
    }

    m_local_endpoint = m_connection->GetLocalEndPoint();

    if (!m_error_code)
	*presult = m_response;
    return m_error_code;
}

unsigned Fetcher::Impl::OnHttpHeader(const std::string& key, 
				     const std::string& value)
{
//    TRACE << "hf" << this << ": " << key << ": " << value << "\n";
    (*m_headers)[key] = value;
    return 0;
}

unsigned Fetcher::Impl::OnHttpData()
{
    unsigned int rc;
    char buffer[1024];

//    TRACE << "In OnHttpData\n";

    do {
	size_t nread;
//	TRACE << "Calling Read\n";
	rc = m_connection->Read(buffer, sizeof(buffer), &nread);
//	TRACE << "Read returned " << rc << "\n";
	if (rc == EWOULDBLOCK)
	    return 0;
	if (rc)
	    return rc;
//	TRACE << "Got " << nread << " bytes\n";
	if (!nread)
	    return 0;

	m_response.append(buffer, nread);
    } while (!m_done);

    return 0;
}

void Fetcher::Impl::OnHttpDone(unsigned int error_code)
{
//    TRACE << "hf" << this << " done (" << error_code << ")\n";
    if (!m_error_code)
	m_error_code = error_code;
    m_done = true;
}


Fetcher::Fetcher(Client *client, 
		 const std::string& url, const char *extra_headers,
		 const char *body,
		 const char *verb)
    : m_impl(new Impl(&m_headers, client, url, extra_headers, body, verb))
{
}

Fetcher::~Fetcher()
{
    delete m_impl;
}

unsigned int Fetcher::FetchToString(std::string *presult)
{
    return m_impl->FetchToString(presult);
}

IPEndPoint Fetcher::GetLocalEndPoint()
{
    return m_impl->GetLocalEndPoint();
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
	    *hostpart = std::string(ptr, (size_t)(slash-ptr));
	}
	else
	{
	    *pathpart = "/";
	    *hostpart = url;
	}
	return;
    }
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

} // namespace http

} // namespace util

#ifdef TEST

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

    util::http::ParseURL(url, &hostpart, &path);
//    TRACE << "hp=" << hostpart << ", path=" << path << "\n";
    assert(hostpart == "http://foo.bar:2888");
    assert(path == "/wurdle");
    
    util::http::ParseHost(hostpart, 80, &host, &port);
//    TRACE << "host=" << host << ", port=" << port << "\n";
    assert(host == "foo.bar");
    assert(port == 2888);

    return 0;
}

#endif
