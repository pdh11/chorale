#include "http_client.h"
#include "config.h"
#include "trace.h"
#include "file.h"
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

namespace util {

HttpClient::HttpClient(const std::string& url)
    : m_url(url)
{
}

static size_t util_http_client_string_callback(void *data,
					       size_t sz, size_t nmemb,
					       void *handle)
{
    std::string *ps = (std::string*) handle;
    size_t realsize = sz*nmemb;
    ps->append((char*)data, realsize);
    return realsize;
}

unsigned int HttpClient::FetchToString(std::string *presult)
{
    *presult = std::string();
    CURL *curl_handle;

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, m_url.c_str());

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, 
		     util_http_client_string_callback);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)presult);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, PACKAGE_STRING);

    /* get it! */
    curl_easy_perform(curl_handle);
    
    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    return 0;
}

static void ParseURL(const std::string& url,
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
	    *hostpart = std::string(ptr, slash-ptr);
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

}; // namespace util

#ifdef TEST

static struct {
    const char *base;
    const char *link;
    const char *expect;
} tests[] = {
    "http://foo.bar/foo", "frink",        "http://foo.bar/frink",
    "http://foo.bar/foo/", "frink",        "http://foo.bar/foo/frink",
    "http://foo.bar/foo/", "/frink",        "http://foo.bar/frink",
    "http://foo.bar",      "wurdle",       "http://foo.bar/wurdle",
    "http://foo.bar",      "/wurdle",       "http://foo.bar/wurdle",
    "http://foo.bar:2888", "wurdle",       "http://foo.bar:2888/wurdle",
    "http://foo.bar:2888","/wurdle",       "http://foo.bar:2888/wurdle",
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
    return 0;
}

#endif
