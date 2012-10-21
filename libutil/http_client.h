#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H 1

#include "stream.h"
#include <string>

namespace util {

class Scheduler;
class IPEndPoint;

/** Classes implementing HTTP/1.1
 */
namespace http {

/** An asynchronous HTTP client connection.
 *
 * For a simple, synchronous fetcher, see util::http::Fetcher. To use
 * the asynchronous API, inherit from Connection and override (some or
 * all of) Write(), OnHeader(), OnEndPoint(), and OnDone(), all of
 * which are called as necessary by the implementation.
 */
class Connection: public util::Stream
{
public:
    virtual ~Connection() {}

    /** Called with data from the returned HTTP body (if transaction
     * succeeds).
     */
    virtual unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    /** Never called (but must be present because we inherit Stream).
     */
    unsigned Read(void*, size_t, size_t*);

    /** Called with each incoming HTTP header (if connection succeeds).
     */
    virtual void OnHeader(const std::string& /*key*/, 
			  const std::string& /*value*/) {}

    /** Called with the *local* IP endpoint the connection got made on.
     */
    virtual void OnEndPoint(const util::IPEndPoint&) {}

    /** Called with the overall result of the connection/transaction attempt.
     */
    virtual void OnDone(unsigned int error_code) = 0;
};

typedef util::CountedPointer<Connection> ConnectionPtr;

/** A central pool of HTTP connections.
 *
 * Declare one somewhere centrally and pass it to everyone who needs
 * an HTTP client.
 */
class Client
{
    class Task;

public:
    Client();

    /** Passing a NULL verb means POST (if body != NULL) or GET (otherwise).
     *
     * The connection may complete before Connect() returns, so your
     * ConnectionPtr should be ready for this. Connection::OnDone is
     * guaranteed to be called at some point if Connect() succeeds
     * (returns 0), whether the transaction succeeds or at whatever
     * point it fails. It is guaranteed not to be called if Connect()
     * returns an error.
     */
    unsigned int Connect(util::Scheduler *poller,
			 ConnectionPtr target,
			 const std::string& url,
			 const std::string& extra_headers = std::string(),
			 const std::string& body = std::string(),
			 const char *verb = NULL);
};

} // namespace http

} // namespace util

#endif
