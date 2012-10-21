#include "config.h"
#include "server.h"
#include "device.h"
#include "soap.h"
#include "ssdp.h"
#include "libutil/trace.h"
#include "libutil/string_stream.h"
#include "libutil/partial_url.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/xml.h"
#include "libutil/xmlescape.h"
#include <sstream>
#include <errno.h>
#include <boost/format.hpp>
#include <boost/thread/tss.hpp>
#if HAVE_WS2TCPIP_H
#include <ws2tcpip.h>  /* For gethostname */
#endif

static const char s_description_path[] = "/upnp/description.xml";

LOG_DECL(SOAP);
LOG_DECL(UPNP);

namespace upnp {

class Server::Impl: public util::http::ContentFactory
{
    Server *m_parent;
    util::Scheduler *m_scheduler;
    util::http::Client *m_client;
    util::http::Server *m_server;
    ssdp::Responder *m_ssdp;

    typedef std::vector<Device*> devices_t;
    devices_t m_devices;

    struct Subscription
    {
	Service *service;
	std::string delivery_url;
	unsigned int event_key;
    };

    typedef std::vector<Subscription> subscriptions_t;
    subscriptions_t m_subscriptions;

    struct SoapInfo
    {
	util::IPEndPoint ipe;
	unsigned int access; ///< As per util::IPFilter
    };

    class SoapReplier;

    boost::thread_specific_ptr<SoapInfo> m_endpoints;

    Service *FindService(const char *udn, const char *service);
    Service *FindService(const char *usn);
    std::string MakeUUID(const std::string& resource);

public:
    Impl(Server*, util::Scheduler *scheduler, util::http::Client *client,
	 util::http::Server *server, ssdp::Responder *ssdp);
    ~Impl();

    unsigned int Init();

    std::string RegisterDevice(Device *d, const std::string& resource);

    std::string DeviceDescription(Device*);
    std::string Description(util::IPAddress);

    void FireEvent(Service *service,
		   const char *variable, const std::string& value);

    util::IPEndPoint GetCurrentEndPoint();
    unsigned int GetCurrentAccess();

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request*, util::http::Response*);
};

Server::Impl::Impl(Server *parent, util::Scheduler *scheduler,
		   util::http::Client *client, util::http::Server *server, 
		   ssdp::Responder *ssdp)
    : m_parent(parent),
      m_scheduler(scheduler),
      m_client(client),
      m_server(server),
      m_ssdp(ssdp)
{
}

Server::Impl::~Impl()
{
    /// @todo Unadvertise
}

Service *Server::Impl::FindService(const char *udn, const char *service_id)
{
    for (devices_t::const_iterator i = m_devices.begin();
	 i != m_devices.end();
	 ++i)
    {
	if ((*i)->GetUDN() == udn)
	    return (*i)->FindServiceByID(service_id);
    }
    return NULL;
}

Service *Server::Impl::FindService(const char *usn)
{
    const char *colons = strstr(usn, "::");
    if (!colons)
	return NULL;
    std::string udn(usn, colons);
    return FindService(udn.c_str(), colons+2);
}

static uint32_t SimpleHash(const char *key)
{
    uint32_t hash = 0;
    while (*key)
    {
	hash += *key++;
	hash += (hash << 10);
	hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/** Construct the UUID for the UPnP Universal Device Name (UDN).
 *
 * In order to get the same UUID each time we run, we bake the hostname
 * and the path into the UUID using a simple hash.
 */
std::string Server::Impl::MakeUUID(const std::string& resource)
{
    union {
	unsigned char ch[16];
	uint32_t ui[4];
    } u;

    char hostname[256];
    hostname[0] = '\0';
    gethostname(hostname, sizeof(hostname));

    strcpy((char*)u.ch, "chorale ");
    u.ui[2] = SimpleHash(hostname);
    u.ui[3] = SimpleHash(resource.c_str());

    char buf[48];
    sprintf(buf, "uuid:%08x-%04x-%04x-%04x-%04x%08x",
	    u.ui[0], u.ui[1] >> 16, u.ui[1] & 0xFFFF, u.ui[2] >> 16, 
	    u.ui[2] & 0xFFFF, u.ui[3]);

    return std::string(buf);
}

std::string Server::Impl::RegisterDevice(Device *d, 
					 const std::string& resource)
{
    m_devices.push_back(d);
    return MakeUUID(resource);
}

std::string Server::Impl::DeviceDescription(Device *d)
{
    std::ostringstream ss;

    /* Note that all these are case-sensitive */

    ss <<
        "<deviceType>" << d->GetDeviceType() << "</deviceType>"
        "<friendlyName>" << d->GetFriendlyName() << "</friendlyName>"
        "<manufacturer>Chorale contributors</manufacturer>"
	"<manufacturerURL>http://chorale.sf.net</manufacturerURL>"
        "<modelDescription>" PACKAGE_VERSION "</modelDescription>"
        "<modelName>" PACKAGE_NAME "</modelName>"
        "<modelNumber>" PACKAGE_VERSION "</modelNumber>"
        "<UDN>" << d->GetUDN() << "</UDN>"
        "<presentationURL>/</presentationURL>"
        "<iconList>"
	"<icon><mimetype>image/png</mimetype>"
        " <width>32</width><height>32</height><depth>24</depth>"
        " <url>/layout/icon.png</url>"
        "</icon>"
	"<icon><mimetype>image/vnd.microsoft.icon</mimetype>"
        " <width>32</width><height>32</height><depth>24</depth>"
        " <url>/layout/icon.ico</url>"
        "</icon>"
	"<icon><mimetype>image/x-icon</mimetype>"
        " <width>32</width><height>32</height><depth>24</depth>"
        " <url>/layout/icon.ico</url>"
        "</icon>"
	"</iconList>"
        "<serviceList>";

    for (Device::const_iterator i = d->begin();
	 i != d->end();
	 ++i)
    {
	ss << "<service>"
	   << "<serviceType>" << (*i)->GetServiceType() << "</serviceType>"
	   << "<serviceId>" << (*i)->GetServiceID() << "</serviceId>"
	   << "<SCPDURL>" << (*i)->GetSCPDUrl() << "</SCPDURL>"
	   << "<controlURL>/upnp/control/" << d->GetUDN() << "::"
	   << (*i)->GetServiceID() << "</controlURL>"
	   << "<eventSubURL>/upnp/event/" << d->GetUDN() << "::"
	   << (*i)->GetServiceID() << "</eventSubURL>"
	   << "</service>"
	    ;
    }
    ss << "</serviceList>";

    return ss.str();
}

std::string Server::Impl::Description(util::IPAddress ip)
{
    devices_t::const_iterator i = m_devices.begin();
    if (i == m_devices.end())
	return "";

    util::IPEndPoint ipe;
    ipe.addr = ip;
    ipe.port = m_server->GetPort();

    std::string s =
	"<?xml version=\"1.0\"?><root xmlns=\"urn:schemas-upnp-org:device-1-0\"><specVersion><major>1</major><minor>0</minor></specVersion>"
	"<URLBase>http://" + ipe.ToString() + "/</URLBase>"
	"<device>"
	+ DeviceDescription(*i);

    ++i;
    if (i != m_devices.end())
    {
	s += "<deviceList>";
	do {
	    s += "<device>";
	    s += DeviceDescription(*i);
	    s += "</device>";
	    ++i;
	} while (i != m_devices.end());
	s += "</deviceList>";
    }
	
    s += "</device>"
	"</root>";

    LOG(UPNP) << "My description is:\n" << s << "\n";

    return s;
}

unsigned int Server::Impl::Init()
{
    util::PartialURL description_url(s_description_path, m_server->GetPort());

    for (devices_t::const_iterator i = m_devices.begin();
	 i != m_devices.end();
	 ++i)
    {
	const Device *device = *i;
	if (i == m_devices.begin())
	    m_ssdp->Advertise("upnp::rootdevice",
			      device->GetUDN(),
			      &description_url);

	m_ssdp->Advertise("",
			  device->GetUDN(), &description_url);

	m_ssdp->Advertise(device->GetDeviceType(), 
			  device->GetUDN(), 
			  &description_url);

	for (Device::const_iterator j = device->begin();
	     j != device->end();
	     ++j)
	{
	    const Service *service = *j;
	    m_ssdp->Advertise(service->GetServiceType(),
			      device->GetUDN(), &description_url);
	}
    }

    m_server->AddContentFactory("/", this);
    return 0;
}

class Notify: public util::http::Connection
{
public:
    Notify() {}
    ~Notify() {}
    
    void OnDone(unsigned int)
    {
//	TRACE << "notify" << (void*)this << " done(" << rc << "), deleting\n";
    }
};

void Server::Impl::FireEvent(Service *service,
			     const char *variable, const std::string& value)
{
    LOG(UPNP) << "Attempting to fire event for " << service->GetServiceType()
	      << "::" << variable
	      << "\n";

    std::string body = "<?xml version=\"1.0\"?>"
	"<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">"
	"<e:property><" + std::string(variable) + ">"
	+ util::XmlEscape(value) + "</" + variable + ">"
	"</e:property>"
	"</e:propertyset>";

    for (subscriptions_t::iterator i = m_subscriptions.begin();
	 i != m_subscriptions.end();
	 ++i)
    {
	if (service == i->service)
	{
//	    TRACE << "Notifying " << i->delivery_url << "\n";
	    std::string extra_headers =
		"NT: upnp:event\r\n"
		"NTS: upnp:propchange\r\n"
		"CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
		"SID: " + i->delivery_url + "\r\n"
		+ (boost::format("SEQ: %u\r\n") % i->event_key).str();

	    i->event_key++;

	    util::http::ConnectionPtr ptr(new Notify);

	    unsigned rc = m_client->Connect(m_scheduler,
					    ptr,
					    i->delivery_url,
					    extra_headers, 
					    body, "NOTIFY");
	    if (rc)
		TRACE << "Can't connect for NOTIFY\n";
	}
    }
}

static bool prefixcmp(const char *haystack, const char *needle, 
		      const char **leftovers)
{
    size_t len = strlen(needle);
    if (!strncmp(haystack, needle, len))
    {
	*leftovers = haystack + len;
	return true;
    }
    return false;
}

class Server::Impl::SoapReplier: public util::Stream,
				 public xml::SaxParserObserver
{
    Service *m_service;
    util::http::Response *m_response;
    util::IPEndPoint m_ipe;
    unsigned int m_access;
    boost::thread_specific_ptr<Server::Impl::SoapInfo> *m_endpoints;
    xml::SaxParser m_parser;
    std::string m_action_name;
    soap::Inbound m_args;
    std::string m_key, m_value;

public:
    SoapReplier(Service *service, util::http::Response *response,
		util::IPEndPoint ipe, unsigned int access,
		boost::thread_specific_ptr<SoapInfo> *endpoints)
	: m_service(service),
	  m_response(response),
	  m_ipe(ipe),
	  m_access(access),
	  m_endpoints(endpoints),
	  m_parser(this)
    {
    }

    ~SoapReplier()
    {
	// End of body -- assemble reply

	SoapInfo *soap_info = new SoapInfo;
	soap_info->ipe = m_ipe;
	soap_info->access = m_access;
	m_endpoints->reset(soap_info);

	soap::Outbound result;
	unsigned int rc = m_service->OnAction(m_action_name.c_str(), m_args,
					      &result);
	if (rc)
	    return;

	std::string body = result.CreateBody(m_action_name + "Response",
					     m_service->GetServiceType());
	
	LOG(SOAP) << "Soap response is " << body << "\n";
	
	m_response->ssp = util::StringStream::Create(body);
    }

    unsigned int OnBegin(const char *tag)
    {
	LOG(SOAP) << "Arg: <" << tag << ">\n";
	if (!strchr(tag, ':'))
	{
	    m_key = tag;
	    m_value.clear();
	}
	else
	{
	    m_key.clear();
	    if (*tag == 'u')
		m_action_name = tag+2;
	}
	return 0;
    }

    unsigned int OnContent(const char *content)
    {
	if (!m_key.empty())
	    m_value += content;
	return 0;
    }

    unsigned int OnEnd(const char*)
    {
	if (!m_key.empty())
	{
	    LOG(SOAP) << "Arg '" << m_key << "' = '" << m_value << "'\n";
	    m_args.Set(m_key, m_value);
	    m_key.clear();
	    m_value.clear();
	}
	return 0;
    }

    // Being a Stream
    unsigned Read(void *, size_t, size_t*) { return EPERM; }
    unsigned Write(const void *buffer, size_t len, size_t *pwrote)
    {
	unsigned int rc = m_parser.WriteAll(buffer, len);
	if (!rc)
	    *pwrote = len;
	return rc;
    }
};

bool Server::Impl::StreamForPath(const util::http::Request *rq, 
				 util::http::Response *rs)
{
    LOG(UPNP) << "Got " << rq->verb << " request for " << rq->path << "\n";

    const char *path = rq->path.c_str();
    const char *usn;

    if (rq->path == s_description_path)
    {
	LOG(UPNP) << "Serving description request\n";
	rs->ssp = util::StringStream::Create(Description(rq->local_ep.addr));
	rs->content_type = "text/xml; charset=\"utf-8\"";
	return true;
    }
    else if (prefixcmp(path, "/upnp/event/", &usn))
    {
	if (rq->verb == "SUBSCRIBE")
	{
	    LOG(UPNP) << "I'm being subscribed at\n" << rq->headers;

	    Service *service = FindService(usn);
	    if (service)
	    {
		std::string delivery = rq->GetHeader("callback");
		if (delivery.size() > 2)
		{
		    delivery.erase(0,1);
		    delivery.erase(delivery.size()-1);

		    Subscription s;
		    s.service = service;
		    s.delivery_url = delivery;
		    s.event_key = 0;

		    m_subscriptions.push_back(s);

		    rs->headers["SID"] = delivery; // why not?
		    rs->headers["TIMEOUT"] = "infinite";
		    rs->ssp = util::StringStream::Create("");
		}
	    }
	    else
	    {
		TRACE << "Can't find service " << usn << "\n";
	    }
	    return true;
	}
    }
    else if (prefixcmp(path, "/upnp/control/", &usn))
    {
	if (rq->verb == "POST")
	{
	    LOG(UPNP) << "I'm being soaped at\n" << rq->headers;
	    Service *svc = FindService(usn);
	    if (svc)
	    {
		rs->content_type = "text/xml; charset=\"utf-8\"";
		rs->body_sink.reset(new SoapReplier(svc, rs, rq->local_ep,
						    rq->access, &m_endpoints));
		return true;
	    }
	    else
	    {
		LOG(UPNP) << "Service " << usn << " not found\n";
	    }
	}
    }

    return false;
}

util::IPEndPoint Server::Impl::GetCurrentEndPoint()
{
    SoapInfo *ptr = m_endpoints.get();
    if (ptr)
	return ptr->ipe;
    util::IPEndPoint nil;
    nil.addr.addr = 0;
    nil.port = 0;
    return nil;
}

unsigned int Server::Impl::GetCurrentAccess()
{
    SoapInfo *ptr = m_endpoints.get();
    assert(ptr);
    return ptr->access;
}


        /* Server */


Server::Server(util::Scheduler *scheduler, 
	       util::http::Client* hc, util::http::Server *hs, 
	       ssdp::Responder *ssdp)
    : m_impl(new Impl(this, scheduler, hc, hs, ssdp))
{
}

Server::~Server()
{
    delete m_impl;
}

unsigned int Server::Init()
{
    return m_impl->Init();
}

void Server::FireEvent(Service *service,
		       const char *variable, const std::string& value)
{
    LOG(UPNP) << "Firing event " << variable << "=" << value << "\n";
    m_impl->FireEvent(service, variable, value);
}

std::string Server::RegisterDevice(Device *device,
				   const std::string& resource)
{
    return m_impl->RegisterDevice(device, resource);
}

util::IPEndPoint Server::GetCurrentEndPoint()
{
    return m_impl->GetCurrentEndPoint();
}

unsigned int Server::GetCurrentAccess()
{
    return m_impl->GetCurrentAccess();
}

} // namespace upnp
