#include "socket.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include "libutil/trace.h"
#undef CTIME

namespace util {

enum { NO_SOCKET = (int)-1 };

static void SetUpSockaddr(const IPEndPoint& ep, struct sockaddr_in *sin)
{
    memset(sin, 0, sizeof(*sin));
    sin->sin_family = AF_INET;
    sin->sin_port = (unsigned short)htons(ep.port);
    sin->sin_addr.s_addr = ep.addr.addr;
}

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
    if (m_fd != NO_SOCKET)
	::close(m_fd);
}

Socket::Socket(const Socket& other)
{
    if (other.m_fd == NO_SOCKET)
	m_fd = NO_SOCKET;
    else
	m_fd = ::dup(other.m_fd);
}

int Socket::GetPollHandle() const
{
    return (int)m_fd;
}

void Socket::operator=(const Socket& other)
{
    if (m_fd != NO_SOCKET)
	::close(m_fd);

    if (other.m_fd == NO_SOCKET)
	m_fd = NO_SOCKET;
    else
	m_fd = ::dup(other.m_fd);
}

unsigned Socket::Bind(const IPEndPoint& ep)
{
    int i = 1;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;

    SetUpSockaddr(ep, &u.sin);

    int rc = ::bind(m_fd, &u.sa, sizeof(u.sin));
    if (rc < 0)
	return (unsigned int)errno;
    return 0;
}

unsigned Socket::Connect(const IPEndPoint& ep)
{
    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;

    SetUpSockaddr(ep, &u.sin);

    int rc = ::connect(m_fd, &u.sa, sizeof(u.sin));
    if (rc < 0)
	return (unsigned int)errno;
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

unsigned Socket::SetNonBlocking(bool nonblocking)
{
    unsigned long whether = nonblocking;
    int rc = ::ioctl(m_fd, FIONBIO, &whether);
    if (rc<0)
	return (unsigned int)errno;
    return 0;
}

bool Socket::IsOpen() const
{
    return (m_fd != NO_SOCKET);
}

unsigned Socket::Close()
{
    int rc = ::close(m_fd);
    m_fd = NO_SOCKET;
    return (rc<0) ? (unsigned int)errno : 0;
}

unsigned Socket::WaitForRead(unsigned int ms)
{
    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLIN;
    
    int rc = ::poll(&pfd, 1, (int)ms);
    if (rc < 0)
	return (unsigned int)errno;
    if (rc == 0)
	return EWOULDBLOCK;
    return 0;
}

unsigned Socket::WaitForWrite(unsigned int ms)
{
    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLOUT;
    
    int rc = ::poll(&pfd, 1, (int)ms);
    if (rc < 0)
	return (unsigned int)errno;
    if (rc == 0)
	return EWOULDBLOCK;
    return 0;
}

class Socket::Stream: public util::Stream
{
    Socket *m_socket;
    unsigned int m_timeout_ms;

public:
    Stream(Socket *socket, unsigned int timeout_ms) 
	: m_socket(socket), m_timeout_ms(timeout_ms) {}

    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
};

unsigned Socket::Stream::Read(void *buffer, size_t len, size_t *pread)
{
    ssize_t rc = ::recv(m_socket->m_fd, buffer, len, MSG_NOSIGNAL);

    if (rc < 0 && errno == EWOULDBLOCK)
    {
	unsigned rc2 = m_socket->WaitForRead(m_timeout_ms);
	if (rc2 != 0)
	    return rc2;
	rc = ::recv(m_socket->m_fd, buffer, len, MSG_NOSIGNAL);
    }

    if (rc < 0)
	return (unsigned int)errno;
    *pread = (size_t)rc;
    return 0;
}

unsigned Socket::Stream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    ssize_t rc = ::send(m_socket->m_fd, buffer, len, MSG_NOSIGNAL);

    if (rc < 0 && errno == EWOULDBLOCK)
    {
	unsigned rc2 = m_socket->WaitForWrite(m_timeout_ms);
	if (rc2 != 0)
	    return rc2;
	rc = ::send(m_socket->m_fd, buffer, len, MSG_NOSIGNAL);
    }

    if (rc < 0)
	return (unsigned int)errno;
    *pwrote = (size_t)rc;
    return 0;
}

StreamPtr Socket::CreateStream(unsigned int timeout_ms)
{
    return StreamPtr(new Socket::Stream(this, timeout_ms));
}


        /* IPAddress */


IPAddress IPAddress::ANY = { 0 };
IPAddress IPAddress::ALL = { (uint32_t)-1 };

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
}

unsigned DatagramSocket::Write(const void *buffer, size_t buflen,
			       const IPEndPoint& to)
{
    union {
	sockaddr_in sin;
	sockaddr sa;
    } u;

    SetUpSockaddr(to, &u.sin);

    ssize_t rc = ::sendto(m_fd, buffer, buflen, 0, &u.sa, sizeof(u.sin));
    if (rc < 0)
	return (unsigned int)errno;

    return 0;
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

unsigned StreamSocket::Open()
{
    m_fd = ::socket(PF_INET, SOCK_STREAM, 0);
    int i = 1;
    ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &i, sizeof(i));
    return 0;
}

unsigned StreamSocket::Listen(unsigned int queue)
{
    int rc = ::listen(m_fd, (int)queue);
    if (rc<0)
	return (unsigned int)errno;
    return 0;
}

unsigned StreamSocket::Accept(StreamSocket *accepted)
{
    struct sockaddr sa;
    socklen_t sl = sizeof(sa);
    int rc = ::accept(m_fd, &sa, &sl);
    if (rc<0)
	return (unsigned int)errno;
//    TRACE << "Accepted fd " << rc << "\n";
    *accepted = StreamSocket(rc);
    return 0;
}

unsigned StreamSocket::SetCork(bool corked)
{
    int i = corked;
    int rc = ::setsockopt(m_fd, IPPROTO_TCP, TCP_CORK, &i, sizeof(i));
    if (rc<0)
	return (unsigned int)errno;
    return 0;
}

} // namespace util
