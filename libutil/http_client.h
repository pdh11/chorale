#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H 1

#include "socket.h"
#include "stream.h"
#include "magic.h"
#include "peeking_line_reader.h"
#include "http_parser.h"
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
class Connection: public util::Stream,
		  public util::Magic<0x34562873>
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

private:
    Client *m_parent;
    Observer *m_observer; /// @todo Observable?
    std::string m_extra_headers;
    std::string m_body;
    const char *m_verb;
    util::IPEndPoint m_remote_endpoint;
    util::IPEndPoint m_local_endpoint;
    util::PollerInterface *m_poller;
    util::StreamSocketPtr m_socket;
    std::string m_host;
    std::string m_path;
    std::string m_headers;
    PeekingLineReader m_line_reader;
    Parser m_parser;

    enum {
	UNINITIALISED,
	CONNECTING,
	SEND_HEADERS,
	SEND_BODY,
	WAITING,
	RECV_HEADERS,
	RECV_BODY,
	IDLE
    } m_state;

    struct {
	size_t transfer_length;
	size_t total_length;
	bool is_range;
	bool got_length;
	bool connection_close;

	void Clear()
	{
	    transfer_length = total_length = 0;
	    is_range = got_length = connection_close = false;
	}
    } m_entity;

    friend class Client;

    Connection(Client *parent,
	       util::PollerInterface *poller,
	       Observer *observer,
	       const std::string& url,
	       const std::string& extra_headers,
	       const std::string& body,
	       const char *verb);


    unsigned int OnActivity();

public:
    ~Connection();

    /** Start the HTTP transaction.
     *
     * Immediate errors (failure to parse host, failure of connect()
     * call) are returned here; errors happening any later come back
     * through Observer::OnHttpDone. In fact, OnHttpDone may be called
     * before Init() returns, i.e. you need to be ready for OnHttpDone
     * calls before you call Init(). If Init() returns a failure,
     * OnHttpDone is guaranteed not to be called afterwards.
     */
    unsigned int Init();

    // Being a Stream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *, size_t, size_t*) { return EINVAL; }
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
