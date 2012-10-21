#include "config.h"
#include "socket.h"
#include "counted_pointer.h"
#include <unistd.h>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <poll.h>
#endif

#include <string.h>
#include <errno.h>
#include <sstream>
#include <unistd.h>
#include "errors.h"
#include "trace.h"

#undef CTIME

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#undef IN
#undef OUT

LOG_DECL(HTTP);

namespace util {

static const struct {
    unsigned from;
    unsigned to;
} errnomap[] = {

#if defined(EAGAIN)
    { EAGAIN, EWOULDBLOCK },
#endif
#if defined(WSAEINVAL)
    { WSAEINVAL, EINVAL },
#endif
#if defined(WSAEISCONN)
    { WSAEISCONN, EISCONN },
#endif
#if defined(WSAEALREADY)
    { WSAEALREADY, EALREADY },
#endif
#if defined(WSAEINPROGRESS)
    { WSAEINPROGRESS, EINPROGRESS },
#endif
#if defined(WSAEWOULDBLOCK)
    { WSAEWOULDBLOCK, EWOULDBLOCK },
#endif

// Nobody likes zero-sized arrays
    { 0, 0 }

};

static unsigned int MappedError(unsigned int e)
{
    for (unsigned int i=0; i<sizeof(errnomap)/sizeof(*errnomap); ++i)
	if (e == errnomap[i].from)
	    return errnomap[i].to;
    return e;
}

#ifdef WIN32
unsigned int SocketError() { return MappedError(WSAGetLastError()); }
#else
unsigned int SocketError() { return MappedError((unsigned int)errno); }
#endif


#ifdef WIN32
class SocketStartup
{
public:
    SocketStartup() { WSADATA junk; WSAStartup(MAKEWORD(2,2), &junk); }
    ~SocketStartup() { WSACleanup(); }
};

static SocketStartup g_ss;
#endif

enum { NO_SOCKET = (int)-1 };

static void SetUpSockaddr(const IPEndPoint& ep, struct sockaddr_in *sin)
{
    memset(sin, 0, sizeof(*sin));
    sin->sin_family = AF_INET;
    sin->sin_port = (unsigned short)htons(ep.port);
    sin->sin_addr.s_addr = ep.addr.addr;
}

Socket::Socket()
    : m_fd(NO_SOCKET),
      m_timeout_ms(5000),
      m_peeked(0)
{
}

Socket::Socket(int fd)
    : m_fd(fd),
      m_timeout_ms(5000),
      m_peeked(0)
{
}

Socket::~Socket()
{
    if (IsOpen())
	Close();
}

unsigned Socket::Bind(const IPEndPoint& ep)
{
    int i = 1;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&i, sizeof(i));

    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;

    SetUpSockaddr(ep, &u.sin);

    int rc = ::bind(m_fd, &u.sa, sizeof(u.sin));
    if (rc < 0)
	return SocketError();
    return 0;
}

unsigned Socket::Connect(const IPEndPoint& ep)
{
    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;

    SetUpSockaddr(ep, &u.sin);

//    TRACE << "Connecting to " << ep.ToString() << "\n";

    int rc = ::connect(m_fd, &u.sa, sizeof(u.sin));
    if (rc < 0)
    {
//	TRACE << "Connect gave error " << rc << " WSAGLE=" << WSAGetLastError()
//	      << "\n";
	return SocketError();
    }
    return 0;
}

IPEndPoint Socket::GetLocalEndPoint()
{
    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;
    socklen_t sinlen = sizeof(u);

    int rc = ::getsockname(m_fd, &u.sa, &sinlen);

    IPEndPoint ep;
    if (rc < 0)
    {
	ep.addr.addr = 0;
	ep.port = 0;
    }
    else
    {
	ep.addr.addr = u.sin.sin_addr.s_addr;
	ep.port = ntohs(u.sin.sin_port);
    }
    return ep;
}

IPEndPoint Socket::GetRemoteEndPoint()
{
    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;
    socklen_t sinlen = sizeof(u);

    int rc = ::getpeername(m_fd, &u.sa, &sinlen);

    IPEndPoint ep;
    if (rc < 0)
    {
	ep.addr.addr = 0;
	ep.port = 0;
    }
    else
    {
	ep.addr.addr = u.sin.sin_addr.s_addr;
	ep.port = ntohs(u.sin.sin_port);
    }
    return ep;
}

unsigned Socket::SetNonBlocking(bool nonblocking)
{
    unsigned long whether = nonblocking;
#ifdef WIN32
    int rc = ::ioctlsocket(m_fd, FIONBIO, &whether);
#else
    int rc = ::ioctl(m_fd, FIONBIO, &whether);
#endif
    if (rc<0)
	return SocketError();
    return 0;
}

unsigned Socket::GetReadable(size_t *preadable)
{
#ifdef WIN32
    unsigned long readable = 0;
    int rc = ::ioctlsocket(m_fd, FIONREAD, &readable);
//    TRACE << "FIONREAD says " << readable << " m_peeked=" << m_peeked << "\n";
#else
    int readable = 0;
    int rc = ::ioctl(m_fd, FIONREAD, &readable);
#endif
    if (rc<0)
	return SocketError();
    *preadable = readable;
    return 0;
}

bool Socket::IsWritable()
{
    fd_set rd, wr, ex;
    FD_ZERO(&rd);
    FD_ZERO(&wr);
    FD_ZERO(&ex);
    struct timeval tv = { 0,0 };

    FD_SET((unsigned)m_fd, &wr);
    FD_SET((unsigned)m_fd, &ex);
    
    int rc = ::select(m_fd+1, &rd, &wr, &ex, &tv);
    if (rc == 0)
	return false;
    return true;
}

bool Socket::IsOpen() const
{
    return (m_fd != NO_SOCKET);
}

unsigned Socket::Close()
{
    if (!IsOpen())
	return 0;
    
    LOG(HTTP) << "Closing " << this << "\n";
#ifdef WIN32
    int rc = ::closesocket(m_fd);
#else
    int rc = ::close(m_fd);
#endif
    m_fd = NO_SOCKET;
    return (rc<0) ? SocketError() : 0;
}

unsigned Socket::WaitForRead(unsigned int ms)
{
#ifdef WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);
    struct timeval tv;
    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    int rc = ::select(m_fd+1, &fds, NULL, NULL, &tv);
    if (rc < 0)
	return SocketError();
    if (rc == 0)
	return EWOULDBLOCK;
    return 0;
#else
//    TRACE << "**** WaitForRead(" << m_fd << ") ****\n";
    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLIN;
    
    int rc = ::poll(&pfd, 1, (int)ms);
    if (rc < 0)
	return SocketError();
    if (rc == 0)
	return EWOULDBLOCK;
    return 0;
#endif
}

unsigned Socket::WaitForWrite(unsigned int ms)
{
#ifdef WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);
    struct timeval tv;
    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    int rc = ::select(m_fd+1, NULL, &fds, &fds, &tv);
    if (rc < 0)
	return SocketError();
    if (rc == 0)
	return EWOULDBLOCK;
    return 0;
#else
//    TRACE << "**** WaitForWrite(" << m_fd << ") ****\n";
    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLOUT;
    
    int rc = ::poll(&pfd, 1, (int)ms);
    if (rc < 0)
	return SocketError();
    if (rc == 0)
	return EWOULDBLOCK;
    return 0;
#endif
}

unsigned Socket::ReadPeek(void *buffer, size_t len, size_t *pread)
{   
#ifdef WIN32
    ssize_t rc = ::recv(m_fd, (char*)buffer, len, MSG_NOSIGNAL|MSG_PEEK);
//    TRACE << "recv(peek) returned " << rc << "\n";

    if (rc < 0)
    {
	unsigned rc3 = SocketError();
//	TRACE << "Socket readpeek failed:" << rc3 << "\n";
	return rc3;
    }
    m_peeked = rc;
    *pread = (size_t)rc;
#else
    ssize_t rc = ::recv(m_fd, (char*)buffer, len, MSG_NOSIGNAL|MSG_PEEK);

    if (rc < 0)
	return (unsigned int)errno;
    m_peeked = (unsigned int)rc;
    *pread = (size_t)rc;
#endif
    return 0;
}

unsigned Socket::Read(void *buffer, size_t len, size_t *pread)
{
#ifdef WIN32
    ssize_t rc = ::recv(m_fd, (char*)buffer, len, MSG_NOSIGNAL);

    if (rc < 0 && WSAGetLastError() == WSAEWOULDBLOCK && m_timeout_ms)
    {
	unsigned rc2 = WaitForRead(m_timeout_ms);
	if (rc2 != 0)
	    return rc2;
	rc = ::recv(m_fd, (char*)buffer, len, MSG_NOSIGNAL);
    }

    if (rc < 0)
    {
//	TRACE << "Socket read failed:" << WSAGetLastError() << "\n";
	return SocketError();
    }
    *pread = (size_t)rc;

    if (rc > (ssize_t)m_peeked)
	m_peeked = 0;
    else
	m_peeked -= rc;
#else
    if (len == 0)
    {
	*pread = 0;
	return 0;
    }

    ssize_t rc = ::recv(m_fd, (char*)buffer, len, MSG_NOSIGNAL);

    if (rc < 0 && errno == EWOULDBLOCK && m_timeout_ms)
    {
	unsigned rc2 = WaitForRead(m_timeout_ms);
	if (rc2 != 0)
	    return rc2;
	rc = ::recv(m_fd, (char*)buffer, len, MSG_NOSIGNAL);
    }

    if (rc < 0)
    {
	return (unsigned int)errno;
    }
    *pread = (size_t)rc;
#endif
    return 0;
}

unsigned Socket::Write(const void *buffer, size_t len, size_t *pwrote)
{
#ifdef WIN32
    ssize_t rc = ::send(m_fd, (const char*)buffer, len,
			MSG_NOSIGNAL);

    if (rc < 0 && WSAGetLastError() == WSAEWOULDBLOCK && m_timeout_ms)
    {
	unsigned rc2 = WaitForWrite(m_timeout_ms);
	if (rc2 != 0)
	    return rc2;
	rc = ::send(m_fd, (const char*)buffer, len, MSG_NOSIGNAL);
    }

    if (rc < 0)
    {
	return SocketError();
    }
    *pwrote = (size_t)rc;
#else
    ssize_t rc = ::send(m_fd, (const char*)buffer, len, MSG_NOSIGNAL);
//    TRACE << "Send() returned " << rc << " errno " << errno << "\n";

    if (rc < 0 && errno == EWOULDBLOCK && m_timeout_ms)
    {
	unsigned rc2 = WaitForWrite(m_timeout_ms);
	if (rc2 != 0)
	    return rc2;
	rc = ::send(m_fd, (const char*)buffer, len, MSG_NOSIGNAL);
    }

    if (rc < 0)
	return (unsigned int)errno;
    *pwrote = (size_t)rc;
#endif
    return 0;
}


        /* IPAddress */


const IPAddress IPAddress::ANY = { 0 };
const IPAddress IPAddress::ALL = { (uint32_t)-1 };

std::string IPAddress::ToString() const
{
    std::ostringstream os;
    uint32_t hostorder = ntohl(addr);
    os << (hostorder >> 24) << "."
       << ((hostorder >> 16) & 0xFF) << "."
       << ((hostorder >> 8) & 0xFF) << "."
       << (hostorder & 0xFF);
    return os.str();
}

IPAddress IPAddress::FromDottedQuad(unsigned char a, unsigned char b,
				    unsigned char c, unsigned char d)
{
    uint32_t hostorder = ((unsigned)a<<24) + ((unsigned)b<<16)
	+ ((unsigned)c<<8) + d;
    IPAddress ip;
    ip.addr = htonl(hostorder);
    return ip;
}

IPAddress IPAddress::FromNetworkOrder(uint32_t netorder)
{
    IPAddress ip;
    ip.addr = netorder;
    return ip;
}

IPAddress IPAddress::Resolve(const char *host)
{
#if HAVE_GETHOSTBYNAME_R
    struct hostent ho;
    char buffer[1024];
    
    struct hostent *result = NULL;
    int herrno;
    int rc = ::gethostbyname_r(host, &ho, buffer, sizeof(buffer),
			       &result, &herrno);
    if (rc < 0)
	return IPAddress::ANY;
    if (!result)
	return IPAddress::ANY;
    
    union {
	char buffer[4];
	uint32_t netorder;
    } u;

    memcpy(u.buffer, result->h_addr, 4);
    return FromNetworkOrder(u.netorder);
#elif HAVE_GETADDRINFO
    struct addrinfo hints;
    struct addrinfo *list = NULL;
    memset(&hints, '\0', sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int rc = ::getaddrinfo(host, 80, &hints, &list);
    if (rc != 0)
	return IPAddress::ANY;

    if (!list)
	return IPAddress::ANY;

    sockaddr_in *sin = (sockaddr_in*)list->ai_addr;
    uint32_t netorder = sin->sin_addr.s_addr;

    ::freeaddrinfo(list);
    
    return FromNetworkOrder(netorder);
#else
    struct hostent *ho = ::gethostbyname(host);
    if (!ho)
	return IPAddress::ANY;

    union {
	char buffer[4];
	uint32_t netorder;
    } u;

    memcpy(u.buffer, ho->h_addr, 4);
    return FromNetworkOrder(u.netorder);
#endif
}


        /* IPEndPoint */


std::string IPEndPoint::ToString() const
{
    std::ostringstream os;
    os << addr.ToString() << ":" << port;
    return os.str();
}


        /* DatagramSocket */


DatagramSocket::DatagramSocket()
    : Socket()
{
    m_fd = ::socket(PF_INET, SOCK_DGRAM, 0);
}

DatagramSocket::~DatagramSocket()
{
}

unsigned DatagramSocket::EnableBroadcast(bool broadcastable)
{
    int i = broadcastable ? 1 : 0;
    int rc = ::setsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, (const char*)&i,
			  sizeof(i));
    if (rc<0)
	return (unsigned int)errno;
    return 0;
}

unsigned DatagramSocket::JoinMulticastGroup(IPAddress multicast_addr,
					    IPAddress interface_addr)
{
    struct ip_mreq mreq;

    memset(&mreq, '\0', sizeof(mreq));

    mreq.imr_multiaddr.s_addr = multicast_addr.addr;
    mreq.imr_interface.s_addr = interface_addr.addr;

    int rc = ::setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			  (const char*)&mreq, sizeof(mreq));

    if (rc < 0)
    {
	TRACE << "JMG failed " << errno << "\n";
	return (unsigned)errno;
    }

    return 0;
}

unsigned DatagramSocket::SetOutgoingMulticastInterface(IPAddress addr)
{
    struct in_addr ina;
    ina.s_addr = addr.addr;
    int rc = ::setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_IF,
			  (const char*)&ina, sizeof(ina));
    if (rc < 0)
    {
	TRACE << "SOMI failed " << errno << "\n";
	return (unsigned)errno;
    }

    return 0;
}

/** Receive a single datagram.
 *
 * For 'wasto', see Stevens UNP vol1 sec 20.2, as refracted via
 * http://www.uwsg.iu.edu/hypermail/linux/net/9810.0/0033.html
 *
 * There is no single-call way to get 'wasto' on non-Linux, but IP_RECVIF
 * and SIOCGIFADDR can do it between them, probably.
 */
unsigned DatagramSocket::Read(void *buffer, size_t buflen, size_t *nread,
			      IPEndPoint *wasfrom, IPAddress *wasto)
{
#ifdef WIN32
    /* Like so many things in Windows, there are multiple ways to do
     * this -- one of which doesn't work, one of which requires you to
     * rewrite your entire program to accommodate it, and the last of
     * which only works on Vista. Here we ignore the problem of getting
     * 'wasto' right.
     */
    WSABUF buf;
    buf.buf = (char*)buffer;
    buf.len = buflen;
    DWORD bytesread = 0;
    union {
	sockaddr sa;
	sockaddr_in sin;
    } u;
    DWORD flags = 0;
    socklen_t fromlen = sizeof(u);

    int rc = WSARecvFrom(m_fd, &buf, 1, &bytesread, &flags, &u.sa, &fromlen,
			 NULL, NULL);
    if (rc != 0)
	return SocketError();
    *nread = bytesread;

    if (m_peeked)
    {
//	TRACE << "Read(2) resets m_peeked\n";
	if (bytesread >= m_peeked)
	    m_peeked = 0;
	else
	    m_peeked -= bytesread;
    }

    if (wasfrom)
    {
	wasfrom->addr.addr = u.sin.sin_addr.s_addr;
	wasfrom->port = ntohs(u.sin.sin_port);
    }

    if (wasto)
    {
	*wasto = IPAddress::ANY;
	char hostname[256];
	if (::gethostname(hostname, sizeof(hostname)) == 0)
	{
//	    TRACE << "GHN succeeded\n";

	    hostent *phost = ::gethostbyname(hostname);
	    // In Win32 the storage is system-owned and thread-local

	    if (phost)
	    {
		*wasto = IPAddress::FromNetworkOrder(((struct in_addr*)(*phost->h_addr_list))->s_addr);
//		TRACE << "GHBN succeeded: " << wasto->ToString() << "\n";
	    }
	}
    }

    return 0;
#else
    union {
	sockaddr sa;
	sockaddr_in sin;
    } u;

    struct msghdr msg;
    msg.msg_name = &u.sa;
    msg.msg_namelen = sizeof(u);
    msg.msg_flags = 0;

    struct iovec iov;
    iov.iov_base = buffer;
    iov.iov_len = buflen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    union {
	struct cmsghdr cm; // For alignment
	char cbuffer[CMSG_SPACE(sizeof(struct in_pktinfo))];
    } u2;
    msg.msg_control = u2.cbuffer;
    msg.msg_controllen = sizeof(u2);
    msg.msg_flags = 0;

    if (wasto)
    {
	int on = 1;
	::setsockopt(m_fd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on));
    }

    ssize_t rc = ::recvmsg(m_fd, &msg, 0);
    if (rc < 0)
	return (unsigned int)errno;
    *nread = (size_t)rc;
    if (wasfrom)
    {
	wasfrom->addr.addr = u.sin.sin_addr.s_addr;
	wasfrom->port = ntohs(u.sin.sin_port);
    }
    if (wasto)
    {
	wasto->addr = 0;
	if (!(msg.msg_flags & MSG_CTRUNC))
	{
	    for (struct cmsghdr *cmptr = CMSG_FIRSTHDR(&msg);
		 cmptr != NULL;
		 cmptr = CMSG_NXTHDR(&msg, cmptr))
	    {
//		TRACE << "level=" << cmptr->cmsg_level 
//		      << " type=" << cmptr->cmsg_type << "\n";
		if (cmptr->cmsg_level == IPPROTO_IP
		    && cmptr->cmsg_type == IP_PKTINFO)
		{
		    struct in_pktinfo ipi;
		    memcpy(&ipi, CMSG_DATA(cmptr), sizeof(ipi));

		    /* ipi.ipi_addr     is the IP address from the packet
		     *                  (ie maybe a broadcast address)
		     * ipi.ipi_spec_dst is the specific destination address
		     *                  (ie unicast address of the if)
		     */
		    
		    wasto->addr = ipi.ipi_spec_dst.s_addr;

//		    TRACE << "ipi_spec_dst="
//			  << IPAddress::FromNetworkOrder(ipi.ipi_spec_dst.s_addr).ToString()
//			  << " ipi_addr="
//			  << IPAddress::FromNetworkOrder(ipi.ipi_addr.s_addr).ToString()
//			  << "\n";
		}
	    }
	}
    }
    return 0;
#endif
}

unsigned DatagramSocket::Read(std::string *s, IPEndPoint *wasfrom, 
			      IPAddress *wasto)
{
#ifdef MSG_TRUNC
    // Note that ioctl(FIONREAD) returns the total size of ALL waiting
    // packets (UNP vol1 sec13.7) which is not what we want.

    char nobuf;
    ssize_t rc = ::recv(m_fd, &nobuf, 1, MSG_PEEK | MSG_TRUNC);
    if (rc < 0)
	return (unsigned)errno;
    if (rc == 0)
    {
	s->clear();
	return 0;
    }

//    TRACE << "MSG_PEEKed " << rc << " bytes\n";

    char *ptr = new char[rc];
    size_t nread;
    unsigned int rc2 = Read(ptr, rc, &nread, wasfrom, wasto);
    if (rc2 == 0)
	s->assign(ptr, ptr+nread);

    delete[] ptr;

    return rc2;
#else
    // Without MSG_TRUNC, we just have to guess

    char buffer[1500];
    size_t nread;
    unsigned int rc = Read(buffer, sizeof(buffer), &nread, wasfrom, wasto);
    if (rc == 0)
	s->assign(buffer, buffer+nread);
    return rc;
#endif
}

unsigned DatagramSocket::Write(const void *buffer, size_t buflen,
			       const IPEndPoint& to)
{
    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;

    SetUpSockaddr(to, &u.sin);

    ssize_t rc = ::sendto(m_fd, (const char*)buffer, buflen, 0, &u.sa, 
			  sizeof(u.sin));
    if (rc < 0)
	return (unsigned int)errno;

    return 0;
}

unsigned DatagramSocket::Write(const std::string& s, const IPEndPoint& to)
{
    return Write(s.data(), s.length(), to);
}


        /* StreamSocket */


StreamSocket::StreamSocket()
    : Socket()
{
    Open();
}

StreamSocket::StreamSocket(int fd)
    : Socket(fd)
{
}

StreamSocketPtr StreamSocket::Create()
{
    return StreamSocketPtr(new StreamSocket); 
}

unsigned StreamSocket::Open()
{
    m_fd = ::socket(PF_INET, SOCK_STREAM, 0);
    int i = 1;
    int rc = ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY,
			  (const char*)&i, sizeof(i));
    if (rc < 0)
    {
	TRACE << "Can't turn off Nagle: " << SocketError() << "\n";
    }

#ifdef WIN32
    /** http://support.microsoft.com/kb/823764/EN-US/
     */
    i = 65537;
    ::setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, (const char*)&i, sizeof(i));
#endif
    return 0;
}

unsigned StreamSocket::Listen(unsigned int queue)
{
    int rc = ::listen(m_fd, (int)queue);
    if (rc<0)
	return SocketError();
    return 0;
}

unsigned StreamSocket::Accept(StreamSocketPtr *accepted)
{
    struct sockaddr sa;
    socklen_t sl = sizeof(sa);
#ifdef WIN32

    SOCKET rc = ::accept(m_fd, &sa, &sl);
    if (rc == INVALID_SOCKET)
    {
	unsigned int error = SocketError();
	TRACE << "Accept failed: " << error << "\n";
	return error;
    }

    /* It starts out eventing the parent socket's event. So disassociate it. */
    WSAEventSelect(rc, NULL, 0);
#else
    int rc = ::accept(m_fd, &sa, &sl);
    if (rc < 0)
	return (unsigned int)errno;
#endif

//    TRACE << "Accepted fd " << rc << "\n";

    *accepted = StreamSocketPtr(new StreamSocket(rc));
    return 0;
}

unsigned StreamSocket::SetCork(bool corked)
{
#if HAVE_TCP_CORK
    int i = corked;
    int rc = ::setsockopt(m_fd, IPPROTO_TCP, TCP_CORK, &i, sizeof(i));
    if (rc<0)
	return SocketError();
#else
    (void)corked;
#endif
    return 0;
}

unsigned StreamSocket::ShutdownWrite()
{
    int how;
#ifdef SD_SEND
    how = SD_SEND;
#elif defined(SHUT_WR)
    how = SHUT_WR;
#else
    return 0;
#endif
    int rc = ::shutdown(m_fd, how);
    if (rc<0)
	return SocketError();
    return 0;
}

} // namespace util
