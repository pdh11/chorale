#include "ssdp.h"
#include "config.h"
#include "libutil/http_parser.h"
#include "libutil/ip_config.h"
#include "libutil/ip_filter.h"
#include "libutil/line_reader.h"
#include "libutil/partial_url.h"
#include "libutil/poll.h"
#include "libutil/bind.h"
#include "libutil/socket.h"
#include "libutil/string_stream.h"
#include "libutil/trace.h"
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#elif defined(HAVE_WS2TCPIP_H)
#include <ws2tcpip.h>
#endif
#include <boost/thread/mutex.hpp>
#include <list>
#include <map>

#undef IN
#undef OUT

namespace upnp {

namespace ssdp {

class Responder::Impl: public util::Timed
{
    util::PollerInterface *m_poller;
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

    typedef std::list<Advertisement*> adverts_t;

    /** List of Advertisement objects
     */
    adverts_t m_adverts;

    util::DatagramSocket m_multicast_socket;
    util::DatagramSocket m_search_socket;

    void SendSearch(const char *uuid);

public:
    Impl(util::PollerInterface*, util::IPFilter*);
    ~Impl();

    unsigned Search(const char *uuid, Callback*);

    unsigned Advertise(const std::string& service_type,
		       const std::string& unique_device_name,
		       const util::PartialURL *url);

    unsigned OnActivity();

    // Being a Timed
    unsigned OnTimer();
};

class Responder::Impl::Advertisement: public util::Timed
{
    util::PollerInterface *m_poller;
    util::IPFilter *m_filter;
    util::DatagramSocket *m_socket;
    std::string m_service_type;
    std::string m_unique_device_name;
    util::PartialURL m_partial_url;

    /** Advertisements are sent in cycles of three */
    unsigned int m_cycle;
    
public:
    Advertisement(util::PollerInterface *poller,
		  util::IPFilter *filter,
		  util::DatagramSocket *socket,
		  const std::string& service_type,
		  const std::string& unique_device_name,
		  const util::PartialURL *partial_url)
	: m_poller(poller),
	  m_filter(filter),
	  m_socket(socket),
	  m_service_type(service_type),
	  m_unique_device_name(unique_device_name),
	  m_partial_url(*partial_url),
	  m_cycle(0)
    {
	m_poller->Add(0,
		      84 + (rand() & 15),
		      this);
    }

    ~Advertisement()
    {
	SendNotify(false);
	m_poller->Remove(this);
    }

    void SendNotify(bool alive=true);

    /** Send a reply to a search
     *
     * @param them Who to reply to (i.e. who we got the search from)
     * @param us   Who to reply from (i.e. address the search was sent to)
     * @param type Service type to claim to be (i.e. the version searched-for)
     */
    void SendReply(const util::IPEndPoint& them,
		   util::IPAddress us, const std::string& type);

    const std::string& GetServiceType() { return m_service_type; }

    /** An advertisement matches a search if either they're exactly the same,
     * or the search is for an earlier version number of the same service.
     */
    bool MatchServiceType(const std::string& search);

    // Being a Timed
    unsigned OnTimer();
};

unsigned Responder::Impl::Advertisement::OnTimer()
{
    SendNotify();
    return 0;
}

void Responder::Impl::Advertisement::SendNotify(bool alive)
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

//	    TRACE << "Sending from " << i->address.ToString() << "\n" << message;
	}
    }

    ++m_cycle;
    if (m_cycle == 3)
    {
	m_cycle = 0;
	m_poller->Remove(this);
	m_poller->Add(time(NULL) + 800*1000,
		      84 + (rand() & 15),
		      this);
    }
}

void Responder::Impl::Advertisement::SendReply(const util::IPEndPoint& them,
					       util::IPAddress us,
					       const std::string& service_type)
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

//    TRACE << "replying to " << them.ToString() << ":\n" << message << "\n";

    m_socket->Write(message, them);
}

bool Responder::Impl::Advertisement::MatchServiceType(const std::string& search)
{
//    TRACE << search << " vs " << m_service_type << "\n";

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


        /* Responder::Impl::Impl */


Responder::Impl::Impl(util::PollerInterface *poller, util::IPFilter *filter)
    : m_poller(poller),
      m_filter(filter)
{
    m_multicast_socket.SetNonBlocking(true);
    m_search_socket.SetNonBlocking(true);
    poller->Add(&m_multicast_socket,
		util::Bind<Impl, &Impl::OnActivity>(this),
		util::PollerInterface::IN);
    poller->Add(&m_search_socket,
		util::Bind<Impl, &Impl::OnActivity>(this),
		util::PollerInterface::IN);

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

Responder::Impl::~Impl()
{
    m_poller->Remove(&m_search_socket);
    m_poller->Remove(&m_multicast_socket);

    while (!m_adverts.empty())
    {
	Advertisement *ad = *m_adverts.begin();
	m_adverts.erase(m_adverts.begin());
	delete ad;
    }
}

unsigned Responder::Impl::OnActivity()
{
//    TRACE << "Activity\n";

    for (;;)
    {
	std::string packet;
	util::IPEndPoint wasfrom;
	util::IPAddress wasto;

	unsigned rc = m_multicast_socket.Read(&packet, &wasfrom, &wasto);
	if (rc != 0 || packet.empty())
	    rc = m_search_socket.Read(&packet, &wasfrom, &wasto);
	if (rc != 0 || packet.empty())
	    return 0;

	if (m_filter
	    && m_filter->CheckAccess(wasfrom.addr) == util::IPFilter::DENY)
	    continue;

//	TRACE << packet;
//	TRACE << "wasto: " << wasto.ToString() << "\n";

	util::StringStreamPtr ssp = util::StringStream::Create(packet);

	// SSDP packets look like HTTP headers
	util::GreedyLineReader glr(ssp);
	util::http::Parser hh(&glr);
	std::string verb, path;
	rc = hh.GetRequestLine(&verb, &path, NULL);

	if (rc)
	{
	    TRACE << "Don't like SSDP header\n" << packet << "\n";
	    continue;
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

//	    TRACE << "search from " << wasfrom.ToString() << " to "
//		  << wasto.ToString() << ": " << service_type << "\n";

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
    }
}

unsigned Responder::Impl::OnTimer()
{
    return 0;
}

unsigned Responder::Impl::Search(const char *uuid, Callback *cb)
{
    {
//	boost::mutex::scoped_lock lock(m_mutex);
	m_map[uuid] = cb;
    }
    SendSearch(uuid);
    return 0;
}

void Responder::Impl::SendSearch(const char *uuid)
{
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

unsigned Responder::Impl::Advertise(const std::string& service_type,
				    const std::string& unique_device_name,
				    const util::PartialURL *url)
{
    m_adverts.push_back(new Advertisement(m_poller,
					  m_filter,
					  &m_search_socket,
					  service_type,
					  unique_device_name,
					  url));
    return 0;
}


        /* Responder itself */


Responder::Responder(util::PollerInterface *poller, util::IPFilter *filter)
    : m_impl(new Impl(poller, filter))
{
}

Responder::~Responder()
{
    delete m_impl;
}

unsigned Responder::Search(const char *uuid, Callback *cb)
{
    return m_impl->Search(uuid, cb);
}

unsigned Responder::Advertise(const std::string& service_type,
			      const std::string& unique_device_name,
			      const util::PartialURL *url)
{
    return m_impl->Advertise(service_type, unique_device_name, url);
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

class MyCallback: public upnp::ssdp::Responder::Callback
{
public:
    void OnService(const std::string& url, const std::string& udn);
};

void MyCallback::OnService(const std::string& url, const std::string& udn)
{
    TRACE << "udn " << udn << " at url " << url << "\n";
}

int main()
{
    util::Poller poller;
    upnp::ssdp::Responder client(&poller, NULL);
    MyCallback cb;
    client.Search(upnp::s_service_type_content_directory, &cb);
    client.Search("urn:chorale-sf-net:device:Test:1", &cb);
    client.Search("urn:chorale-sf-net:device:Test:3", &cb);

    util::PartialURL purl("/description.xml", 80);
    client.Advertise("urn:chorale-sf-net:device:Test:2",
		     "uuid:00-00-00-00",
		     &purl);

    time_t t = time(NULL) + 5;

    while (time(NULL) < t)
    {
	poller.Poll(1000);
    }

    return 0;
}
#endif
