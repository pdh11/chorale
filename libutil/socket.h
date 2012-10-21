#ifndef LIBUTIL_SOCKET_H
#define LIBUTIL_SOCKET_H

#include <inttypes.h>
#include <stdlib.h>
#include <string>
#include <boost/noncopyable.hpp>
#include "stream.h"
#include "config.h"

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

    bool operator==(const IPAddress& other) { return addr == other.addr; }
    bool operator!=(const IPAddress& other) { return addr != other.addr; }
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
class Socket: public Stream
{
protected:
    int m_fd;
    void *m_event;
    enum { NONE, READ, WRITE } m_event_type;
    /** Timeout for synchronous operations */
    unsigned int m_timeout_ms;

    /** How many bytes have we Peek'd but not yet read? */
    unsigned int m_peeked;

    Socket();
    ~Socket();
    explicit Socket(int fd);
   
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
    IPEndPoint GetRemoteEndPoint();

    unsigned Connect(const IPEndPoint&);

    PollHandle GetReadHandle();
    PollHandle GetWriteHandle();

    bool IsOpen() const;
    unsigned Close();

    /** Reads up to len bytes without removing them from the input buffer.
     *
     * Note that on Windows (http://support.microsoft.com/kb/140263) you can't
     * just go on peeking waiting for enough to arrive; you have to do a real
     * read and then peek again.
     */
    unsigned ReadPeek(void *buffer, size_t len, size_t *pread);

    /** Get amount of data in input buffer (i.e. how much we can Read)
     */
    unsigned GetReadable(size_t *preadable);

    // Being a Stream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    /** Set the (automatic) timeout on Read and Write. Default is 5000ms.
     *
     * Clients which deal with EWOULDBLOCK themselves can use 0.
     */
    void SetTimeoutMS(unsigned int ms) { m_timeout_ms = ms; }
};

/** UDP socket
 *
 * Note that a UDP socket isn't quite like other Streams, in that Read() has
 * the UDP recv() semantics.
 */
class DatagramSocket: public Socket
{
public:
    DatagramSocket();
    ~DatagramSocket();

    unsigned EnableBroadcast(bool broadcastable);

    /** Join a multicast group.
     *
     * Note that you need to do this on all the interfaces you care about
     * @sa http://tldp.org/HOWTO/Multicast-HOWTO-6.html
     */
    unsigned JoinMulticastGroup(IPAddress multicast_addr,
				IPAddress interface_addr);

    unsigned SetOutgoingMulticastInterface(IPAddress);

    using Stream::Read;
    unsigned Read(void *buffer, size_t buflen, size_t *nread,
		  IPEndPoint *wasfrom, IPAddress *wasto);
    unsigned Read(std::string*, IPEndPoint *wasfrom, IPAddress *wasto);

    using Stream::Write;
    unsigned Write(const void *buffer, size_t buflen, const IPEndPoint& to);
    unsigned Write(const std::string&, const IPEndPoint& to);
};

/** TCP socket */
class StreamSocket: public Socket
{
    explicit StreamSocket(int fd);

    StreamSocket();
public:
    typedef boost::intrusive_ptr<StreamSocket> StreamSocketPtr;

    static StreamSocketPtr Create() { return new StreamSocket; }
    
    unsigned Listen(unsigned int queue = 64);
    unsigned Accept(StreamSocketPtr *accepted);
   
    unsigned Open();

    unsigned SetCork(bool corked);
};

typedef boost::intrusive_ptr<StreamSocket> StreamSocketPtr;

} // namespace util

#endif
