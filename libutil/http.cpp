#include "http.h"
#include "file.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

namespace util {

namespace http {


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

# include <stdlib.h>
# include "trace.h"

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

    return 0;
}

#endif
