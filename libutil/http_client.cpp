#include "config.h"
#include "http_client.h"
#include "http_stream.h"
#include "string_stream.h"
#include "trace.h"
#include "file.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

namespace util {

HttpClient::HttpClient(const std::string& url)
    : m_url(url)
{
}

unsigned int HttpClient::FetchToString(std::string *presult)
{
    *presult = std::string();

    StringStreamPtr output = StringStream::Create();
    HTTPStreamPtr input;
    unsigned int rc = HTTPStream::Create(&input, m_url.c_str());
    if (rc != 0)
	return rc;
    rc = CopyStream(SeekableStreamPtr(input), output);
    if (rc != 0)
	return rc;
    
   *presult = output->str();
    return 0;
}

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
	std::string result = util::ResolveURL(tests[i].base, tests[i].link);
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

    util::ParseURL(url, &hostpart, &path);
//    TRACE << "hp=" << hostpart << ", path=" << path << "\n";
    assert(hostpart == "http://foo.bar:2888");
    assert(path == "/wurdle");
    
    util::ParseHost(hostpart, 80, &host, &port);
//    TRACE << "host=" << host << ", port=" << port << "\n";
    assert(host == "foo.bar");
    assert(port == 2888);

    return 0;
}

#endif
