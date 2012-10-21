#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H 1

#include "counted_object.h"
#include <string>

namespace util {

class Scheduler;
class IPEndPoint;

namespace http {

/** A recipient of data from an asynchronous HTTP client connection.
 *
 * For a simple, synchronous fetcher, see util::http::Fetcher. To use
 * the asynchronous API, inherit from Connection and override (some or
 * all of) Write(), OnHeader(), OnEndPoint(), and OnDone(), all of
 * which are called as necessary by the implementation.
 */
class Recipient: public util::CountedObject<>
{
public:
    virtual ~Recipient() {}

    /** Called with data from the returned HTTP body (if transaction
     * succeeds).
     */
    virtual unsigned OnData(const void *buffer, size_t len) = 0;

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

typedef CountedPointer<Recipient> RecipientPtr;

/** A central pool of HTTP connections.
 *
 * Declare one somewhere centrally and pass it to everyone who needs
 * an HTTP client.
 */
class Client
{
    class Task;
    std::string m_useragent_header;

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
			 RecipientPtr recipient,
			 const std::string& url,
			 const std::string& extra_headers = std::string(),
			 const std::string& body = std::string(),
			 const char *verb = NULL);
};

} // namespace http

} // namespace util

#endif
