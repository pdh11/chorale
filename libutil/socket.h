#ifndef LIBUTIL_SOCKET_H
#define LIBUTIL_SOCKET_H

#include <string>
#include <memory>
#include <stdint.h>
#include "stream.h"
#include "ip.h"
#include "attributes.h"

namespace util {

/** Thin veneer round BSD-style sockets API.
 *
 * Contains only calls useful to both UDP and TCP sockets.
 */
class Socket: public Stream
{
protected:
    int m_fd;

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

    // Being a Pollable
    int GetHandle() { return m_fd; }

    /** Returns stream flags.
     *
     * Note that a socket, by itself, isn't WAITABLE, as it has no connection
     * to a scheduler or task queue. Only a higher-level object such as an
     * http::Client can be WAITABLE.
     */
    unsigned GetStreamFlags() const { return READABLE|WRITABLE|POLLABLE; }

    bool IsOpen() const;
    unsigned Close();

    /** Get amount of data in input buffer (i.e. how much we can Read)
     */
    unsigned GetReadable(size_t *preadable);

    /** Is the socket writable (e.g. has nonblocking connect succeeded?)
     */
    bool IsWritable();

    // Being a Stream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    struct Buffer
    {
	const void *ptr;
	size_t len;
    };
    unsigned WriteV(const Buffer *buffers, unsigned int nbuffers,
		    size_t *pwrote);
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
    StreamSocket(int fd);

public:
    StreamSocket();
    ~StreamSocket();
    
    unsigned Listen(unsigned int queue = 64);
    unsigned Accept(std::auto_ptr<StreamSocket> *accepted);
   
    unsigned Open();

    unsigned SetCork(bool corked);

    /** TCP half-close: we will read no more from this socket
     */
    unsigned ShutdownRead();

    /** TCP half-close: we will write no more to this socket
     */
    unsigned ShutdownWrite();
};

} // namespace util

#endif
