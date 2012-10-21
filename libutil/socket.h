#ifndef LIBUTIL_SOCKET_H
#define LIBUTIL_SOCKET_H

#include <inttypes.h>
#include <stdlib.h>
#include <string>
#include <boost/noncopyable.hpp>
#include "stream.h"

namespace util {

struct IPAddress
{
    // Network byte order
    uint32_t addr;

    static IPAddress FromDottedQuad(unsigned char a, unsigned char b,
				    unsigned char c, unsigned char d);

    /** Blocking call, could take some time */
    static IPAddress Resolve(const char *hostname);

    static IPAddress FromHostOrder(uint32_t);
    static IPAddress FromNetworkOrder(uint32_t);
    static IPAddress ANY;
    static IPAddress ALL;

    std::string ToString() const;
};

struct IPEndPoint
{
    IPAddress      addr;
    unsigned short port; ///< Host byte order

    std::string ToString() const;
};

/** Thin veneer round BSD-style sockets API.
 *
 * Contains only calls useful to both UDP and TCP sockets.
 */
class Socket: public boost::noncopyable
{
protected:
    int m_fd;
    void *m_event;

    Socket();
    ~Socket();
    explicit Socket(int fd);

    class Stream;
    friend class Stream;
   
public:
    unsigned SetNonBlocking(bool nonblocking);

    /** Wait, up to the given length of time, for the socket to become
     * readable.
     *
     * Return code: 0 for readable, EWOULDBLOCK for not-readable, other errors
     * probably serious.
     */
    unsigned WaitForRead(unsigned int ms);

    /** Wait, up to the given length of time, for the socket to become
     * writable.
     *
     * Return code: 0 for writable, EWOULDBLOCK for not-writable, other errors
     * probably serious.
     */
    unsigned WaitForWrite(unsigned int ms);

    unsigned Bind(const IPEndPoint&);
    IPEndPoint GetLocalEndPoint();

    unsigned Connect(const IPEndPoint&);

    /** direction is PollerInterface::IN and/or OUT
     */
    int GetPollHandle(unsigned int direction);

    bool IsOpen() const;
    unsigned Close();

    StreamPtr CreateStream(unsigned int timeout_ms = 5000);
};

/** UDP socket */
class DatagramSocket: public Socket
{
public:
    DatagramSocket();
    DatagramSocket(const DatagramSocket& other);

    ~DatagramSocket();

    unsigned EnableBroadcast(bool broadcastable);

    unsigned Read(void *buffer, size_t buflen, size_t *nread,
		  IPEndPoint *wasfrom, IPAddress *wasto);

    unsigned Write(const void *buffer, size_t buflen, const IPEndPoint& to);
};

/** TCP socket */
class StreamSocket: public Socket
{
    explicit StreamSocket(int fd);

public:
    StreamSocket();
    
    unsigned Listen(unsigned int queue = 64);
    unsigned Accept(StreamSocket *accepted);
   
    unsigned Open();

    unsigned SetCork(bool corked);
};

} // namespace util

#endif
