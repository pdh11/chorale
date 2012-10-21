#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H 1

#include "socket.h"
#include "stream.h"
#include <string>

namespace util {

class PollerInterface;

/** Classes implementing HTTP/1.1
 */
namespace http {

class Client;

/** An asynchronous HTTP client connection.
 *
 * For a simple, synchronous fetcher, see util::http::Fetcher.
 */
class Connection: public util::Stream
{
public:
    class Observer
    {
    public:
	virtual ~Observer() {}

	virtual unsigned OnHttpHeader(const std::string& key,
				      const std::string& value) = 0;
	virtual unsigned OnHttpData() = 0;
	virtual void OnHttpDone(unsigned int error_code) = 0;
    };

    IPEndPoint GetLocalEndPoint() { return m_local_endpoint; }

protected:
    util::IPEndPoint m_local_endpoint;

    Connection();

public:
    /** Start the HTTP transaction.
     *
     * Immediate errors (failure to parse host, failure of connect()
     * call) are returned here; errors happening any later come back
     * through Observer::OnHttpDone. In fact, OnHttpDone may be called
     * before Init() returns, i.e. you need to be ready for OnHttpDone
     * calls before you call Init(). If Init() returns a failure,
     * OnHttpDone is guaranteed not to be called afterwards.
     */
    virtual unsigned int Init() = 0;
};

typedef boost::intrusive_ptr<Connection> ConnectionPtr;

/** A central pool of HTTP connections.
 *
 * Declare one somewhere centrally and pass it to everyone who needs
 * an HTTP client.
 */
class Client
{
public:
    Client();

    /** Passing a NULL verb means POST (if body != NULL) or GET (otherwise).
     *
     * Note that the connection doesn't actually start until you call Init()
     * on the returned object.
     */
    ConnectionPtr Connect(util::PollerInterface *poller,
			  Connection::Observer *obs,
			  const std::string& url,
			  const std::string& extra_headers = std::string(),
			  const std::string& body = std::string(),
			  const char *verb = NULL);
};

} // namespace http

} // namespace util

#endif
