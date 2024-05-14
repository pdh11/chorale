#include "config.h"
#include "socket.h"
#include "counted_pointer.h"
#include <unistd.h>

#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <poll.h>
#if HAVE_NET_IF_H
# include <net/if.h>
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

namespace {

const struct {
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

unsigned int MappedError(unsigned int e)
{
    for (unsigned int i=0; i<sizeof(errnomap)/sizeof(*errnomap); ++i)
	if (e == errnomap[i].from)
	    return errnomap[i].to;
    return e;
}

unsigned int SocketError() { return MappedError((unsigned int)errno); }


enum { NO_SOCKET = (int)-1 };

void SetUpSockaddr(const IPEndPoint& ep, struct sockaddr_in *sin)
{
    memset(sin, 0, sizeof(*sin));
    sin->sin_family = AF_INET;
    sin->sin_port = (unsigned short)htons(ep.port);
    sin->sin_addr.s_addr = ep.addr.addr;
}

} // anon namespace

Socket::Socket()
    : m_fd(NO_SOCKET)
{
}

Socket::Socket(int fd)
    : m_fd(fd)
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
    int rc = ::ioctl(m_fd, FIONBIO, &whether);
    if (rc<0)
	return SocketError();
    return 0;
}

unsigned Socket::GetReadable(size_t *preadable)
{
    int readable = 0;
    int rc = ::ioctl(m_fd, FIONREAD, &readable);
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
    int rc = ::close(m_fd);
    m_fd = NO_SOCKET;
    return (rc<0) ? SocketError() : 0;
}

unsigned Socket::WaitForRead(unsigned int ms)
{
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
}

unsigned Socket::WaitForWrite(unsigned int ms)
{
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
}

unsigned Socket::Read(void *buffer, size_t len, size_t *pread)
{
    if (len == 0)
    {
	*pread = 0;
	return 0;
    }

    ssize_t rc = ::recv(m_fd, (char*)buffer, len, MSG_NOSIGNAL);

    if (rc < 0)
    {
	return (unsigned int)errno;
    }
    *pread = (size_t)rc;
    return 0;
}

unsigned Socket::Write(const void *buffer, size_t len, size_t *pwrote)
{
    ssize_t rc = ::send(m_fd, (const char*)buffer, len, MSG_NOSIGNAL);
//    TRACE << "Send() returned " << rc << " errno " << errno << "\n";

    if (rc < 0)
	return (unsigned int)errno;
    *pwrote = (size_t)rc;
    return 0;
}

unsigned Socket::WriteV(const Buffer *buffers, unsigned int nbuffers,
			size_t *pwrote)
{
    *pwrote = 0;
    ssize_t rc = writev(m_fd, (const struct iovec*)buffers, nbuffers);
    if (rc < 0)
	return (unsigned int)errno;
    *pwrote = (size_t)rc;
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
    int rc = ::getaddrinfo(host, "80", &hints, &list);
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
#if HAVE_IP_PKTINFO
	char cbuffer[CMSG_SPACE(sizeof(struct in_pktinfo))];
#elif HAVE_IP_RECVIF
	char cbuffer[CMSG_SPACE(sizeof(struct sockaddr_dl))];
#endif
    } u2;
    msg.msg_control = u2.cbuffer;
    msg.msg_controllen = sizeof(u2);
    msg.msg_flags = 0;

    if (wasto)
    {
	int on = 1;
#if HAVE_IP_PKTINFO
	::setsockopt(m_fd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on));
#elif HAVE_IP_RECVIF
	::setsockopt(m_fd, IPPROTO_IP, IP_RECVIF, &on, sizeof(on));
#endif
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
#if HAVE_IP_PKTINFO
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
#elif HAVE_IP_RECVIF
		if (cmptr->cmsg_level == IPPROTO_IP
		    && cmptr->cmsg_type == IP_RECVIF)
		{
		    struct sockaddr_dl sdl;
		    memcpy(&sdl, CMSG_DATA(cmptr), sizeof(sdl));
		    
		    TRACE << "Interface " << sdl.sdl_index << "\n";
		    if (sdl.sdl_index && !sdl.sdl_nlen)
		    {
			if (!if_indextoname(sdl.sdl_index, sdl.sdl_data))
			    continue;
			sdl.sdl_nlen = strlen(sdl.sdl_data);
		    }

		    struct ifreq ifr;
		    memset(&ifr, '\0', sizeof(ifr));
		    memcpy(ifr.ifr_name, sdl.sdl_data, sdl.sdl_nlen);
		    
		    if (ioctl(m_fd, SIOCGIFADDR, &ifr) < 0)
			continue;

		    u.sa = ifr.ifr_ifru.ifru_addr;
		    wasto->addr = u.sin.sin_addr.s_addr;
		}
#endif
	    }
	}
    }
    return 0;
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
    size_t nread = 0;
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

StreamSocket::~StreamSocket()
{
}

unsigned StreamSocket::Open()
{
    m_fd = ::socket(PF_INET, SOCK_STREAM, 0);
    return 0;
}

unsigned StreamSocket::Listen(unsigned int queue)
{
    int rc = ::listen(m_fd, (int)queue);
    if (rc<0)
	return SocketError();
    return 0;
}

unsigned StreamSocket::Accept(std::unique_ptr<StreamSocket> *accepted)
{
    struct sockaddr sa;
    socklen_t sl = sizeof(sa);
    int rc = ::accept(m_fd, &sa, &sl);
    if (rc < 0)
	return (unsigned int)errno;

//    TRACE << "Accepted fd " << rc << "\n";

    accepted->reset(new StreamSocket(rc));
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
