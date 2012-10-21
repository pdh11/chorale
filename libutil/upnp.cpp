#include "config.h"
#include "upnp.h"
#include "trace.h"

#ifdef HAVE_UPNP

#include <list>
#include <boost/thread/recursive_mutex.hpp>
#ifndef WIN32
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#endif
#include "socket.h"

#include <upnp/upnp.h>
#include <upnp/ThreadPool.h>

namespace util {

static boost::recursive_mutex s_mutex;
static std::list<LibUPnPUser*> s_list;
static UpnpClient_Handle s_handle = 0;

static unsigned int DeduceLocalIPAddress(std::string *ips)
{
#ifdef WIN32
    return ENOSYS;
#else
    int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
    {
	TRACE << "Can't create socket\n";
	return (unsigned)errno;
    }
    
    enum { MAX_IFS = 32 }; // Not found a valid one after 32? give up

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

    for (unsigned int i=0; i<n; ++i)
    {
	struct ifreq *req = &reqs[i];

	req->ifr_flags = 0;
	ioctl(fd, SIOCGIFFLAGS, req);
//	TRACE << "interface " << req->ifr_name << " flags " << req->ifr_flags
//	      << "\n";
    }

    size_t found = n;
    
    for (unsigned int i=0; i<n; ++i)
    {
	reqs[i].ifr_flags = 0;
	ioctl(fd, SIOCGIFFLAGS, &reqs[i]);
	unsigned int flags = (unsigned int)reqs[i].ifr_flags;

	rc = ioctl(fd, SIOCGIFADDR, &reqs[i]);
	if (rc < 0)
	{
//	    TRACE << reqs[i].ifr_name << " has no address, skipping\n";
	    continue;
	}

	/* Prefer to have IFF_UP and IFF_RUNNING */ 
	if ((flags & (IFF_UP|IFF_RUNNING|IFF_LOOPBACK)) 
	    == (IFF_UP|IFF_RUNNING))
	{
	    // Definitely like this one
//	    TRACE << reqs[i].ifr_name << " is up and running, selecting\n";
	    found = i;
	    break;
	}
	      
	/* But settle for just IFF_UP */
	if ((flags & (IFF_UP|IFF_LOOPBACK)) == IFF_UP && found == n)
	{
//	    TRACE << reqs[i].ifr_name
//		  << " is up but not running, best so far\n";
	    found = i;
	}
    }

    if (found == n)
    {
	TRACE << "No up interfaces found\n";
    }
    else
    {
	union {
	    sockaddr sa;
	    sockaddr_in sin;
	} u;
	u.sa = reqs[found].ifr_ifru.ifru_addr;
	IPAddress ip = IPAddress::FromNetworkOrder(u.sin.sin_addr.s_addr);
	*ips = ip.ToString();
//	TRACE << "Selecting " << reqs[found].ifr_name << ": " << *ips << "\n";
    }

    ::close(fd);
    return (found == n) ? ENOENT : 0u;
#endif // !WIN32
}

LibUPnPUser::LibUPnPUser()
{
    boost::recursive_mutex::scoped_lock lock(s_mutex);
    if (s_list.empty())
    {
	std::string myip;
	unsigned int rc = DeduceLocalIPAddress(&myip);

	int rc2 = UpnpInit(rc == 0 ? myip.c_str() : NULL, 0);
	if (rc2 != 0)
	{
	    TRACE << "UpnpInit failed (" << rc2 << ")\n";
	}
    }
    s_list.push_back(this);
}

LibUPnPUser::~LibUPnPUser()
{
    boost::recursive_mutex::scoped_lock lock(s_mutex);
    s_list.remove(this);
    if (s_list.empty())
    {
	if (s_handle)
	    UpnpUnRegisterClient(s_handle);
	s_handle = 0;
	    
	UpnpFinish();
    }
}

static int StaticCallback(Upnp_EventType et, void* event, void *)
{
    boost::recursive_mutex::scoped_lock lock(s_mutex);
    for (std::list<LibUPnPUser*>::const_iterator i = s_list.begin();
	 i != s_list.end();
	 ++i)
    {
	(*i)->OnUPnPEvent(et, event);
    }

    return UPNP_E_SUCCESS;
}

size_t LibUPnPUser::GetHandle()
{
    if (!s_handle)
    {
	int rc = UpnpRegisterClient(&StaticCallback, NULL,
				    &s_handle);
	if (rc != UPNP_E_SUCCESS)
	    TRACE << "URC failed: " << rc << "\n";
    }
    return (size_t)s_handle;
}

int LibUPnPUser::OnUPnPEvent(int, void*)
{
    return UPNP_E_SUCCESS;
}

} // namespace util

#endif // HAVE_UPNP
