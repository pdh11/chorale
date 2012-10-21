#include "ip_config.h"
#include "trace.h"

#ifdef WIN32

#include <ws2tcpip.h>

#ifndef IFF_RUNNING
#define IFF_RUNNING 0
#endif

namespace util {

unsigned IPConfig::GetInterfaceList(Interfaces *interfaces)
{
    interfaces->clear();

    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (s == INVALID_SOCKET)
    {
	TRACE << "Can't create socket\n";
	return WSAGetLastError();
    }
    
    enum { MAX_IFS = 32 };

    INTERFACE_INFO reqs[MAX_IFS];
    DWORD bytes_returned;
    int rc = WSAIoctl(s, SIO_GET_INTERFACE_LIST, 
		      NULL, 0,
		      reqs, sizeof(reqs),
		      &bytes_returned, NULL, NULL);
    ::closesocket(s);
    if (rc)
    {
	TRACE << "Can't SIO_GET_INTERFACE_LIST\n";
	return WSAGetLastError();
    }

//    TRACE << bytes_returned << " bytes, size=" << sizeof(INTERFACE_INFO) << "\n";

    unsigned int n = bytes_returned / sizeof(INTERFACE_INFO);

    for (unsigned int i=0; i<n; ++i)
    {
	Interface iface;

//	TRACE << "flags=" << reqs[i].iiFlags << "\n";

	iface.flags = (unsigned int)reqs[i].iiFlags;
	iface.address = IPAddress::FromNetworkOrder(
	    reqs[i].iiAddress.AddressIn.sin_addr.s_addr);

	// This is probably a Wine bug
	if (iface.address == IPAddress::ALL)
	    continue;

	iface.broadcast = IPAddress::FromNetworkOrder(
	    reqs[i].iiBroadcastAddress.AddressIn.sin_addr.s_addr);
	iface.netmask = IPAddress::FromNetworkOrder(
	    reqs[i].iiNetmask.AddressIn.sin_addr.s_addr);
	// Interfaces don't seem to have names in win32

	interfaces->push_back(iface);
    }

    return 0;
}

} // namespace util

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#include <poll.h>

namespace util {

unsigned IPConfig::GetInterfaceList(Interfaces *interfaces)
{
    interfaces->clear();

    int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
    {
	TRACE << "Can't create socket\n";
	return (unsigned)errno;
    }
    
    enum { MAX_IFS = 32 };

    struct ifreq reqs[MAX_IFS];
    struct ifconf ifc;
    ifc.ifc_len = sizeof(reqs);
    ifc.ifc_buf = (char*)&reqs[0];
    int rc = ::ioctl(fd, SIOCGIFCONF, &ifc);
    if (rc < 0)
    {
	TRACE << "Can't SIOCGIFCONF\n";
	::close(fd);
	return (unsigned)errno;
    }
    const size_t n = ((unsigned)ifc.ifc_len) / sizeof(reqs[0]);

    union {
	sockaddr sa;
	sockaddr_in sin;
    } u;
    
    for (unsigned int i=0; i<n; ++i)
    {
	Interface iface;

	reqs[i].ifr_flags = 0;
	ioctl(fd, SIOCGIFFLAGS, &reqs[i]);
	iface.flags = (unsigned int)reqs[i].ifr_flags;

	rc = ioctl(fd, SIOCGIFADDR, &reqs[i]);
	if (rc < 0)
	{
//	    TRACE << reqs[i].ifr_name << " has no address, skipping\n";
	    continue;
	}
	iface.name = reqs[i].ifr_name;
	u.sa = reqs[i].ifr_ifru.ifru_addr;

	iface.address = IPAddress::FromNetworkOrder(u.sin.sin_addr.s_addr);

	ioctl(fd, SIOCGIFBRDADDR, &reqs[i]);
	u.sa = reqs[i].ifr_ifru.ifru_addr;
	iface.broadcast = IPAddress::FromNetworkOrder(u.sin.sin_addr.s_addr);

	ioctl(fd, SIOCGIFNETMASK, &reqs[i]);
	u.sa = reqs[i].ifr_ifru.ifru_addr;
	iface.netmask = IPAddress::FromNetworkOrder(u.sin.sin_addr.s_addr);

	interfaces->push_back(iface);
    }
    ::close(fd);
    return 0;
}

} // namespace util

#endif // !WIN32

#ifdef TEST

int main()
{
    util::IPConfig::Interfaces interfaces;

    unsigned int rc = util::IPConfig::GetInterfaceList(&interfaces);
    assert(rc == 0);

    for (util::IPConfig::Interfaces::const_iterator i = interfaces.begin();
	 i != interfaces.end();
	 ++i)
    {
	unsigned int flags = i->flags;
	TRACE << "if " << i->address.ToString()
	      << " bcast " << i->broadcast.ToString()
	      << " netmask " << i->netmask.ToString()
	      << ((flags & IFF_UP) ? " up" : "")
	      << ((flags & IFF_RUNNING) ? " running" : "")
	      << ((flags & IFF_BROADCAST) ? " bcast" : "")
	      << ((flags & IFF_MULTICAST) ? " mcast" : "")
	      << "\n";
    }

    return 0;
}

#endif
