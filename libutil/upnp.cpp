#include "config.h"
#include "upnp.h"
#include "libutil/trace.h"
#include <list>
#include <upnp/upnp.h>
#include <upnp/ThreadPool.h>
#include <boost/thread/recursive_mutex.hpp>

namespace util {

static boost::recursive_mutex s_mutex;
static std::list<LibUPnPUser*> s_list;
static UpnpClient_Handle s_handle = 0;

LibUPnPUser::LibUPnPUser()
{
    boost::recursive_mutex::scoped_lock lock(s_mutex);
    if (s_list.empty())
    {
//	TRACE << "Calling UpnpInit\n";
	UpnpInit(NULL, 0);
//	TRACE << "UpnpInit done\n";
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
	if (rc == UPNP_E_SUCCESS)
	    TRACE << "URC success\n";
	else
	    TRACE << "URC failed: " << rc << "\n";
    }
    return s_handle;
}

int LibUPnPUser::OnUPnPEvent(int, void*)
{
    return UPNP_E_SUCCESS;
}

}; // namespace util
