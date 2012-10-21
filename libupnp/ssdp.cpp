#include "ssdp.h"
#include "config.h"
#include "libutil/http_parser.h"
#include "libutil/ip_config.h"
#include "libutil/ip_filter.h"
#include "libutil/line_reader.h"
#include "libutil/partial_url.h"
#include "libutil/scheduler.h"
#include "libutil/bind.h"
#include "libutil/socket.h"
#include "libutil/string_stream.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#if HAVE_NET_IF_H
#include <net/if.h>
#elif HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#include <list>
#include <map>
#include <set>
#include <string.h>
#include <stdlib.h>

#undef IN
#undef OUT

LOG_DECL(SSDP);

namespace upnp {

namespace ssdp {

class Responder::Task: public util::Task
{
    util::Scheduler *m_scheduler;
    util::IPFilter *m_filter;

    typedef std::map<std::string, Callback*> map_t;

    /** The remote uuids (NTs) that we're looking for
     */
    map_t m_map;

    typedef std::pair<std::string, std::string> pair_t;
    typedef std::set<pair_t> set_t;

    /** Set of remote udn/service pairs that we've seen so far
     */
    set_t m_set;

    class Advertisement;
    friend class Advertisement;
    typedef util::CountedPointer<Advertisement> AdvertisementPtr;

    typedef std::list<AdvertisementPtr> adverts_t;

    /** List of Advertisement objects
     */
    adverts_t m_adverts;

    util::DatagramSocket m_multicast_socket;
    util::DatagramSocket m_search_socket;

    void SendSearch(const char *uuid);

    unsigned OnMulticastActivity();
    unsigned OnSearchActivity();
    unsigned OnPacket(const std::string& packet,
		      util::IPEndPoint wasfrom,
		      util::IPAddress wasto);

public:
    Task(util::Scheduler*, util::IPFilter*);
    ~Task();

    unsigned Search(const char *uuid, Callback*);

    unsigned Advertise(const std::string& service_type,
		       const std::string& unique_device_name,
		       const util::PartialURL *url);

    unsigned Run() { return 0; } // Delete me once Task::Run goes away

    void Shutdown();
};

class Responder::Task::Advertisement: public util::Task
{
    util::Scheduler *m_scheduler;
    util::IPFilter *m_filter;
    util::DatagramSocket *m_socket;
    std::string m_service_type;
    std::string m_unique_device_name;
    util::PartialURL m_partial_url;

    /** Advertisements are sent in cycles of three */
    unsigned int m_cycle;
    
public:
    Advertisement(util::Scheduler *scheduler,
		  util::IPFilter *filter,
		  util::DatagramSocket *socket,
		  const std::string& service_type,
		  const std::string& unique_device_name,
		  const util::PartialURL *partial_url)
	: m_scheduler(scheduler),
	  m_filter(filter),
	  m_socket(socket),
	  m_service_type(service_type),
	  m_unique_device_name(unique_device_name),
	  m_partial_url(*partial_url),
	  m_cycle(0)
    {
	m_scheduler->Wait(
	    util::Bind<Advertisement,&Advertisement::OnTimer>(AdvertisementPtr(this)),
	    0, 84 + (rand() & 15));
    }

    ~Advertisement()
    {
    }

    void SendNotify(bool alive=true);

    /** Send a reply to a search
     *
     * @param them Who to reply to (i.e. who we got the search from)
     * @param us   Who to reply from (i.e. address the search was sent to)
     * @param type Service type to claim to be (i.e. the version searched-for)
     */
    void SendReply(const util::IPEndPoint& them,
		   util::IPAddress us, const std::string& type) const;

    const std::string& GetServiceType() const { return m_service_type; }

    /** An advertisement matches a search if either they're exactly the same,
     * or the search is for an earlier version number of the same service.
     */
    bool MatchServiceType(const std::string& search) const;

    unsigned OnTimer();

    unsigned Run() { return 0; } // Delete me once Task::Run goes away

    void Shutdown();
};

unsigned Responder::Task::Advertisement::OnTimer()
{
    SendNotify();
    return 0;
}

void Responder::Task::Advertisement::Shutdown()
{
    m_scheduler->Remove(AdvertisementPtr(this));
    SendNotify(false);
}

void Responder::Task::Advertisement::SendNotify(bool alive)
{
    /** We need to send the advert out with different URLs on different
     * interfaces.
     */
    util::IPConfig::Interfaces interface_list;
    
    util::IPConfig::GetInterfaceList(&interface_list);

    for (util::IPConfig::Interfaces::const_iterator i = interface_list.begin();
	 i != interface_list.end();
	 ++i)
    {
	if (i->flags & IFF_MULTICAST)
	{
	    if (m_filter
		&& m_filter->CheckAccess(i->address) == util::IPFilter::DENY)
		continue;

	    m_socket->SetOutgoingMulticastInterface(i->address);

	    std::string nt = m_service_type.empty()
		? m_unique_device_name
		: m_service_type;
	    std::string usn = m_service_type.empty()
		? m_unique_device_name
		: m_unique_device_name + "::" + m_service_type;
	
	    /** @todo Server OS */
	    std::string message = "NOTIFY * HTTP/1.1\r\n"
		"HOST: 239.255.255.250:1900\r\n"
		"CACHE-CONTROL: max-age=1800\r\n"
		"LOCATION: " + m_partial_url.Resolve(i->address) + "\r\n"
		"NT: " + nt + "\r\n"
		"NTS: " + (alive ? "ssdp:alive" : "ssdp:byebye") + "\r\n"
		"SERVER: UPnP/1.0 " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n"
		"USN: " + usn + "\r\n"
		"\r\n";
	
	    util::IPEndPoint ipe;
	    ipe.addr = util::IPAddress::FromDottedQuad(239,255,255,250);
	    ipe.port = 1900;
	    m_socket->Write(message, ipe);

	    LOG(SSDP) << "Sending from " << i->address.ToString() << "\n" << message;
	}
    }

    ++m_cycle;
    if (m_cycle == 3)
    {
	m_cycle = 0;
	m_scheduler->Remove(util::TaskPtr(this));
	m_scheduler->Wait(
	    util::Bind<Advertisement,&Advertisement::OnTimer>(AdvertisementPtr(this)),
	    time(NULL) + 800*1000,
	    84 + (rand() & 15));
    }
}

void Responder::Task::Advertisement::SendReply(const util::IPEndPoint& them,
					       util::IPAddress us,
					       const std::string& service_type) const
{
    std::string usn = service_type.empty()
	? m_unique_device_name
	: m_unique_device_name + "::" + m_service_type;

    /** @todo DATE, server OS */
    std::string message = "HTTP/1.1 200 OK\r\n"
	"CACHE-CONTROL: max-age=1800\r\n"
	"EXT:\r\n"
	"LOCATION: " + m_partial_url.Resolve(us) + "\r\n"
	"SERVER:  UPnP/1.0 " PACKAGE_NAME "/" PACKAGE_VERSION "\r\n"
	"ST: " + service_type + "\r\n"
	"USN: " + usn + "\r\n"
	"\r\n";

    LOG(SSDP) << "replying to " << them.ToString() << ":\n" << message << "\n";

    m_socket->Write(message, them);
}

bool Responder::Task::Advertisement::MatchServiceType(const std::string& search) const
{
    LOG(SSDP) << search << " vs " << m_service_type << "\n";

    if (search == m_service_type)
    {
//	TRACE << "Complete match\n";
	return true;
    }
    
    std::string::size_type versionpos = m_service_type.find_last_not_of("0123456789");
    if (versionpos == m_service_type.size()
	|| versionpos == m_service_type.size()-1
	|| m_service_type[versionpos] != ':')
	return false; // Not versioned

    if (search.size() <= versionpos
	|| search[versionpos] != ':')
	return false; // Search not versioned or wrong length

    if (search.compare(0, versionpos, m_service_type, 0, versionpos))
	return false; // Search doesn't match

    unsigned long my_version = strtoul(m_service_type.c_str() + versionpos+1,
				       NULL, 10);
    unsigned long search_version =  strtoul(search.c_str() + versionpos+1,
					    NULL, 10);

    if (my_version > search_version)
    {
	TRACE << "Returning downgrade version " << search_version << " of "
	      << m_service_type << "\n";
    }
//    TRACE << "Searching for version " << search_version << " I'm version "
//	  << my_version  << "\n";
    return search_version <= my_version;
}


        /* Responder::Task::Task */


Responder::Task::Task(util::Scheduler *scheduler, util::IPFilter *filter)
    : m_scheduler(scheduler),
      m_filter(filter)
{
    m_multicast_socket.SetNonBlocking(true);
    m_search_socket.SetNonBlocking(true);
    scheduler->WaitForReadable(
	util::Bind<Task, &Task::OnMulticastActivity>(TaskPtr(this)),
	&m_multicast_socket, false);
    scheduler->WaitForReadable(
	util::Bind<Task, &Task::OnSearchActivity>(TaskPtr(this)),
	&m_search_socket, false);

    util::IPEndPoint ipe;
    ipe.addr = util::IPAddress::ANY;
    ipe.port = 1900;
    m_multicast_socket.Bind(ipe);

    util::IPConfig::Interfaces interface_list;
    
    util::IPConfig::GetInterfaceList(&interface_list);

    for (util::IPConfig::Interfaces::const_iterator i = interface_list.begin();
	 i != interface_list.end();
	 ++i)
    {
	if (i->flags & IFF_MULTICAST)
	{
	    unsigned rc = m_multicast_socket.JoinMulticastGroup(
		util::IPAddress::FromDottedQuad(239,255,255,250),
		i->address);
	    if (rc)
	    {
		TRACE << "SSDP can't multicast on " << i->name << ": "
		      << rc << "\n";
	    }
	}
    }
}

Responder::Task::~Task()
{
}

void Responder::Task::Shutdown()
{
    for (adverts_t::iterator i = m_adverts.begin();
	 i != m_adverts.end();
	 ++i)
    {
	(*i)->Shutdown();
    }	
    m_scheduler->Remove(util::TaskPtr(this));
}

unsigned Responder::Task::OnMulticastActivity()
{
    for (;;)
    {
	std::string packet;
	util::IPEndPoint wasfrom;
	util::IPAddress wasto;

	unsigned rc = m_multicast_socket.Read(&packet, &wasfrom, &wasto);
	if (rc == 0)
	    OnPacket(packet, wasfrom, wasto);
	else if (rc != EWOULDBLOCK)
	    return rc;
	else
	    return 0;
    }
}

unsigned Responder::Task::OnSearchActivity()
{
    for (;;)
    {
	std::string packet;
	util::IPEndPoint wasfrom;
	util::IPAddress wasto;

	unsigned rc = m_search_socket.Read(&packet, &wasfrom, &wasto);
	if (rc == 0)
	    OnPacket(packet, wasfrom, wasto);
	else if (rc != EWOULDBLOCK)
	    return rc;
	else
	    return 0;
    }
}


unsigned Responder::Task::OnPacket(const std::string& packet, 
				   util::IPEndPoint wasfrom,
				   util::IPAddress wasto)
{
    if (m_filter
	&& m_filter->CheckAccess(wasfrom.addr) == util::IPFilter::DENY)
	return 0;

    LOG(SSDP) << packet;
    LOG(SSDP) << "wasto: " << wasto.ToString() << "\n";

    util::StringStreamPtr ssp = util::StringStream::Create(packet);

    // SSDP packets look like HTTP headers
    util::GreedyLineReader glr(ssp);
    util::http::Parser hh(&glr);
    std::string verb, path;
    unsigned int rc = hh.GetRequestLine(&verb, &path, NULL);

    if (rc)
    {
	TRACE << "Don't like SSDP header\n" << packet << "\n";
	return 0;
    }

    if (verb == "NOTIFY" || path == "200")
    {
	std::string service, location, udn;
	bool byebye = false;
	
	std::string key, value;
	do {
	    rc = hh.GetHeaderLine(&key, &value);

	    if (!strcasecmp(key.c_str(), "ST")
		|| !strcasecmp(key.c_str(), "NT"))
	    {
		if (!value.empty())
		    service = value;
	    }
	    else if (!strcasecmp(key.c_str(), "Location"))
		location = value;
	    else if (!strcasecmp(key.c_str(), "USN"))
	    {
		// We're meant to parse the USN to find the UDN
		std::string::size_type colons = value.find("::");
		if (colons != std::string::npos)
		    udn.assign(value, 0, colons);
	    }
	    else if (!strcasecmp(key.c_str(), "NTS"))
	    {
		if (value == "ssdp:byebye")
		    byebye = true;
	    }
	} while (!key.empty());

//	    TRACE << (verb == "NOTIFY" ? "notify" : "reply") << " from " << wasfrom.ToString() << " to "
//		  << wasto.ToString() << ": " << service << "\n";

	if (!service.empty() && !location.empty() && !udn.empty())
	{
//		TRACE << "Service " << service << " found at '"
//		      << location << "' in id '" << udn << "'\n";

	    if (m_map.find(service) != m_map.end())
	    {
		pair_t us = std::make_pair(udn, service);
		
		if (byebye)
		{
		    if (m_set.find(us) != m_set.end())
		    {
			m_set.erase(us);
			m_map[service]->OnServiceLost(location, udn);
		    }
		}
		else
		{
		    if (m_set.find(us) == m_set.end())
		    {
			m_set.insert(us);
			m_map[service]->OnService(location, udn);
		    }
		}
	    }
	}
    }
    else if (verb == "M-SEARCH")
    {
	std::string key, value;
	std::string service_type, search_id;
	do {
	    rc = hh.GetHeaderLine(&key, &value);
	    if (!strcasecmp(key.c_str(), "ST"))
		service_type = value;
	} while (!key.empty());

	LOG(SSDP) << "search from " << wasfrom.ToString() << " to "
		  << wasto.ToString() << ": " << service_type << "\n";

	for (adverts_t::const_iterator i = m_adverts.begin();
	     i != m_adverts.end();
	     ++i)
	{
	    /** 
	     * @todo Check we advertise everything we should (UPnP-DA p21)
	     */
	    if (service_type == "ssdp:all")
		(*i)->SendReply(wasfrom, wasto, (*i)->GetServiceType());
	    else if ((*i)->MatchServiceType(service_type))
		(*i)->SendReply(wasfrom, wasto, service_type);
	}	   
    }
    else
    {
	TRACE << "Didn't like SSDP verb " << verb << "\n";
    }
    return 0;
}

unsigned Responder::Task::Search(const char *uuid, Callback *cb)
{
    {
//	boost::mutex::scoped_lock lock(m_mutex);
	m_map[uuid] = cb;
    }
    SendSearch(uuid);
    return 0;
}

void Responder::Task::SendSearch(const char *uuid)
{
    util::IPConfig::Interfaces interface_list;
    
    util::IPConfig::GetInterfaceList(&interface_list);

    for (util::IPConfig::Interfaces::const_iterator i = interface_list.begin();
	 i != interface_list.end();
	 ++i)
    {
	if (i->flags & IFF_MULTICAST)
	{
	    if (m_filter
		&& m_filter->CheckAccess(i->address) == util::IPFilter::DENY)
		continue;

	    m_search_socket.SetOutgoingMulticastInterface(i->address);

	    std::string search("M-SEARCH * HTTP/1.1\r\n"
			       "Host: 239.255.255.250:1900\r\n"
			       "Man: \"ssdp:discover\"\r\n"
			       "MX: 3\r\n"
			       "ST: ");
	    search += uuid;
	    search += "\r\n"
		"\r\n";
	    util::IPEndPoint ipe;
	    ipe.addr = util::IPAddress::FromDottedQuad(239,255,255,250);
	    ipe.port = 1900;
	    m_search_socket.Write(search, ipe);
	}
    }
}

unsigned Responder::Task::Advertise(const std::string& service_type,
				    const std::string& unique_device_name,
				    const util::PartialURL *url)
{
    m_adverts.push_back(AdvertisementPtr(new Advertisement(m_scheduler,
							   m_filter,
							   &m_search_socket,
							   service_type,
							   unique_device_name,
							   url)));
    return 0;
}


        /* Responder itself */


Responder::Responder(util::Scheduler *scheduler, util::IPFilter *filter)
    : m_task(new Task(scheduler, filter))
{
}

Responder::~Responder()
{
    m_task->Shutdown();
    m_task.reset(NULL);
}

unsigned Responder::Search(const char *uuid, Callback *cb)
{
    return m_task->Search(uuid, cb);
}

unsigned Responder::Advertise(const std::string& service_type,
			      const std::string& unique_device_name,
			      const util::PartialURL *url)
{
    return m_task->Advertise(service_type, unique_device_name, url);
}


} // namespace ssdp


        /* The UUIDs */

const char s_device_type_media_renderer[] = 
    "urn:schemas-upnp-org:device:MediaRenderer:1";

const char s_device_type_optical_drive[] = 
    "urn:chorale-sf-net:device:OpticalDrive:1";

const char s_service_id_optical_drive[] =
     "urn:chorale-sf-net:serviceId:OpticalDrive";

const char s_service_id_content_directory[] =
    "urn:upnp-org:serviceId:ContentDirectory";

const char s_service_id_av_transport[] =
    "urn:upnp-org:serviceId:AVTransport";

const char s_service_id_connection_manager[] =
    "urn:upnp-org:serviceId:ConnectionManager";
    
const char s_service_id_rendering_control[] =
    "urn:upnp-org:serviceId:RenderingControl";

const char s_service_type_optical_drive[] =
     "urn:chorale-sf-net:service:OpticalDrive:1";

const char s_service_type_content_directory[] =
    "urn:schemas-upnp-org:service:ContentDirectory:1";

const char s_service_type_av_transport[] =
    "urn:schemas-upnp-org:service:AVTransport:1";

const char s_service_type_connection_manager[] =
    "urn:schemas-upnp-org:service:ConnectionManager:1";


} // namespace upnp


#ifdef TEST

std::map<std::string, std::string> g_map;

class MyCallback: public upnp::ssdp::Responder::Callback
{
public:
    void OnService(const std::string& url, const std::string& udn);
};

void MyCallback::OnService(const std::string& url, const std::string& udn)
{
//    TRACE << "udn " << udn << " at url " << url << "\n";
    g_map[udn] = url;
}

int main()
{
    util::BackgroundScheduler poller;
    upnp::ssdp::Responder client(&poller, NULL);
    MyCallback cb;
    client.Search(upnp::s_service_type_content_directory, &cb);
    client.Search("urn:chorale-sf-net:device:Test:1", &cb);
    client.Search("urn:chorale-sf-net:device:Test:3", &cb);

    util::PartialURL purl("/description.xml", 80);
    client.Advertise("urn:chorale-sf-net:device:Test:2",
		     "uuid:00-00-00-00",
		     &purl);

    time_t t = time(NULL);

    while ((time(NULL) - t) < 5)
    {
	poller.Poll(1000);
    }

    std::string url = g_map["uuid:00-00-00-00"];
    assert(!url.empty());

    return 0;
}
#endif
