#ifndef LIBUTIL_HTTP_FETCHER_H
#define LIBUTIL_HTTP_FETCHER_H 1

#include <string>
#include <map>
#include "ip.h"

namespace util {

namespace http {

class Client;

/** Simple, synchronous HTTP fetcher.
 */
class Fetcher
{
    Client *m_client;
    std::string m_url;
    const char *m_extra_headers;
    const char *m_body;
    const char *m_verb;

    std::map<std::string, std::string> m_headers;
    IPEndPoint m_local_endpoint;

    class Task;

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
	return m_headers[name]; /// @bug case-sensitive
    }

    /** Returns the local IP address which was used to contact the server.
     *
     * This is mainly useful on multi-homed hosts.
     */
    IPEndPoint GetLocalEndPoint() const { return m_local_endpoint; }
};

} // namespace http

} // namespace util

#endif
