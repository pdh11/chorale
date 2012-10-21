#ifndef LIBUTIL_SOCKET_H
#define LIBUTIL_SOCKET_H

#include <inttypes.h>
#include <stdlib.h>
#include <string>
#include "stream.h"

namespace util {

struct IPAddress
{
    // Network byte order
    uint32_t addr;

    static IPAddress FromDottedQuad(unsigned char a, unsigned char b,
				    unsigned char c, unsigned char d);

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
class Socket
{
protected:
    size_t m_fd;

    Socket();
    ~Socket();
    explicit Socket(size_t fd);

    Socket(const Socket& other);
    void operator=(const Socket& other);

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

    unsigned Bind(const IPEndPoint&);
    IPEndPoint GetLocalEndPoint();

    int GetPollHandle() const;

    bool IsOpen() const;
    unsigned Close();

    StreamPtr CreateStream();
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
    explicit StreamSocket(size_t fd);

public:
    StreamSocket();
    
    unsigned Listen(unsigned queue = 64);
    unsigned Accept(StreamSocket *accepted);
};

}; // namespace util

#endif
