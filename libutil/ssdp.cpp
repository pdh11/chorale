#include "ssdp.h"
#include "upnp.h"
#include "poll.h"
#include <upnp/upnp.h>
#include <upnp/ixml.h>
#include <map>
#include <boost/thread/recursive_mutex.hpp>
#include "trace.h"

namespace util {

namespace ssdp {

class Client::Impl: public LibUPnPUser, public Pollable
{
    boost::recursive_mutex m_mutex;
    typedef std::map<std::string, Callback*> map_t;
    map_t m_map;

    typedef std::pair<std::string, std::string> pair_t;

    typedef std::set<pair_t> set_t;
    /** Set of udn/service pairs seen so far */
    set_t m_set;

    /** Set of service/location pairs not yet communicated to fg thread */
    set_t m_new;

    PollWaker* m_waker;

public:
    explicit Impl(util::PollerInterface*);
    ~Impl();

    unsigned Init(const char *uuid, Callback*);

    // Being a Pollable
    unsigned OnActivity();

    // Being a LibUPnPUser
    int OnUPnPEvent(int, void*);
};

Client::Impl::Impl(util::PollerInterface *poller)
    : m_waker(NULL)
{
    if (poller)
	m_waker = new PollWaker(poller, this);
}

Client::Impl::~Impl()
{
    delete m_waker;
}

unsigned Client::Impl::Init(const char *uuid, Callback *cb)
{
    boost::recursive_mutex::scoped_lock lock(m_mutex);
    m_map[uuid] = cb;
    TRACE << "Calling USA\n";
    int rc = UpnpSearchAsync(GetHandle(), 100, uuid, this);
    TRACE << "USA: " << rc << "\n";
    return 0;
}

int Client::Impl::OnUPnPEvent(int et, void *event)
{
//    TRACE << "UPNP callback: " << et << "\n";

    switch (et)
    {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
	Upnp_Discovery *disc = (Upnp_Discovery*)event;
	if (disc->ServiceType[0])
	{
//	    TRACE << "Service " << disc->ServiceType << " found at '"
//		  << disc->Location << "' in id '" << disc->DeviceId << "'\n";
	    boost::recursive_mutex::scoped_lock lock(m_mutex);
	    if (m_map.find(disc->ServiceType) != m_map.end())
	    {
		pair_t us = std::make_pair(disc->DeviceId, disc->ServiceType);

		if (m_set.find(us) == m_set.end())
		{
		    m_set.insert(us);
		    pair_t sl 
			= std::make_pair(disc->ServiceType, disc->Location);
		    if (m_waker)
		    {
			m_new.insert(sl);
			m_waker->Wake();
		    }
		    else
		    {
			// Async
			m_map[disc->ServiceType]->OnService(disc->Location);
		    }
		}
//		else
//		    TRACE << "Duplicate service\n";
	    }
//	    else
//		TRACE << "Unknown service\n";
	}
	else
	{
//	    TRACE << "Device " << disc->DeviceType << " found at '" 
//		  << disc->Location << "'\n";
	}
	break;
    }
    default:
	break;
    }

    return UPNP_E_SUCCESS;
}

unsigned Client::Impl::OnActivity()
{
    set_t newservices;
    {
	boost::recursive_mutex::scoped_lock lock(m_mutex);
	newservices.swap(m_new);
    }
    for (set_t::const_iterator i = newservices.begin();
	 i != newservices.end();
	 ++i)
    {
	TRACE << "Communicating " << i->second << " to fg\n";
	m_map[i->first]->OnService(i->second);
    }
    return 0;
}

Client::Client(util::PollerInterface *poller)
    : m_impl(new Impl(poller))
{
}

Client::~Client()
{
    delete m_impl;
}

unsigned Client::Init(const char *uuid,
		      Callback *cb)
{
    TRACE << "Calling impl->init\n";
    unsigned int rc = m_impl->Init(uuid, cb);
    TRACE << "init exiting\n";
    return rc;
}

const char *const s_uuid_contentdirectory =
    "urn:schemas-upnp-org:service:ContentDirectory:1";

const char *const s_uuid_avtransport =
    "urn:schemas-upnp-org:service:AVTransport:1";

const char *const s_uuid_connectionmanager =
    "urn:schemas-upnp-org:service:ConnectionManager:1";

}; // namespace ssdp

}; // namespace util

#ifdef TEST

class MyCallback: public util::ssdp::Client::Callback
{
public:
    void OnService(const std::string& url);
};

void MyCallback::OnService(const std::string& url)
{
    IXML_Document *xmldoc = NULL;
    TRACE << "downloading " << url << "\n";
    int rc = UpnpDownloadXmlDoc(url.c_str(), &xmldoc);
    TRACE << "UDXD " << rc << "\n";

    if (rc == 0)
    {
	DOMString ds = ixmlPrintDocument(xmldoc);
	TRACE << ds << "\n";
	ixmlFreeDOMString(ds);
    }

    if (xmldoc)
	ixmlDocument_free(xmldoc);
}

int main()
{
    util::ssdp::Client client(NULL);
    MyCallback cb;
    client.Init(util::ssdp::s_uuid_contentdirectory, &cb);

    sleep(5);
    
    return 0;
}
#endif
