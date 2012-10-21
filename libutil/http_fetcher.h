#ifndef LIBUTIL_HTTP_FETCHER_H
#define LIBUTIL_HTTP_FETCHER_H 1

#include <string>
#include <map>

namespace util {

class IPEndPoint;

namespace http {

class Client;

/** Simple, synchronous HTTP fetcher.
 */
class Fetcher
{
    std::map<std::string, std::string> m_headers;

    class Impl;
    Impl *m_impl;

public:
    Fetcher(Client *client,
	    const std::string& url,
	    const char *extra_headers = NULL,
	    const char *body = NULL, 
	    const char *verb = NULL);
    ~Fetcher();

    unsigned int FetchToString(std::string *presult);

    std::string GetHeader(const std::string& name)
    {
	return m_headers[name]; // @bug case-sensitive
    }

    /** Returns the local IP address used to contact the server.
     *
     * This is mainly useful on multi-homed hosts.
     */
    IPEndPoint GetLocalEndPoint();
};

std::string ResolveURL(const std::string& base, const std::string& link);

void ParseURL(const std::string& url, std::string *host,
	      std::string *path);

void ParseHost(const std::string& hostpart, unsigned short default_port,
	       std::string *hostname, unsigned short *port);

} // namespace http

} // namespace util

#endif
