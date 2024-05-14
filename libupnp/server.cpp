#include "config.h"
#include "server.h"
#include "device.h"
#include "soap.h"
#include "ssdp.h"
#include "data.h"
#include "soap_parser.h"
#include "libutil/trace.h"
#include "libutil/string_stream.h"
#include "libutil/partial_url.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/printf.h"
#include "libutil/xml.h"
#include "libutil/xmlescape.h"
#include <sstream>
#include <errno.h>
#include <stdio.h>
#include <boost/thread/tss.hpp>

static const char s_description_path[] = "/upnp/description.xml";

LOG_DECL(SOAP);
LOG_DECL(UPNP);

namespace upnp {

class Server::Impl: public util::http::ContentFactory
{
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

        Subscription(Service *s, const std::string& url)
            : service(s), delivery_url(url), event_key(0) {}
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

Server::Impl::Impl(Server*, util::Scheduler *scheduler,
		   util::http::Client *client, util::http::Server *server, 
		   ssdp::Responder *ssdp)
    : m_scheduler(scheduler),
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

    memcpy(u.ch, "chorale ", 8);
    u.ui[2] = SimpleHash(hostname);
    u.ui[3] = SimpleHash(resource.c_str());

    return util::SPrintf("uuid:%08x-%04x-%04x-%04x-%04x%08x",
	    u.ui[0], u.ui[1] >> 16, u.ui[1] & 0xFFFF, u.ui[2] >> 16, 
	    u.ui[2] & 0xFFFF, u.ui[3]);
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
	"<modelURL>http://chorale.sf.net</modelURL>"
        "<UDN>" << d->GetUDN() << "</UDN>"
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
	"<?xml version=\"1.0\"?><root xmlns=\"urn:schemas-upnp-org:device-1-0\" configId=\"1\"><specVersion><major>1</major><minor>0</minor></specVersion>"
	"<device>"
	+ DeviceDescription(*i);

    ++i;
    if (i != m_devices.end())
    {
	s += "<deviceList>";
	do {
	    s += "<device>";
	    s += DeviceDescription(*i);
	    s += "<presentationURL>/</presentationURL></device>";
	    ++i;
	} while (i != m_devices.end());
	s += "</deviceList>";
    }
	
    s += "<presentationURL>/</presentationURL></device>"
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

	assert(!device->GetUDN().empty());

	if (i == m_devices.begin())
	    m_ssdp->Advertise("upnp:rootdevice",
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

class Notify: public util::http::Recipient
{
public:
    Notify() {}
    ~Notify() {}
    
    unsigned OnData(const void*, size_t) { return 0; }
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
		util::Printf() <<
		"NT: upnp:event\r\n"
		"NTS: upnp:propchange\r\n"
		"CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
		"SID: " << i->delivery_url << "\r\n"
		"SEQ: " << i->event_key << "\r\n";

	    TRACE << extra_headers;

	    i->event_key++;

	    util::http::RecipientPtr ptr(new Notify);

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

class Server::Impl::SoapReplier: public util::Stream
{
    Service *m_service;
    util::http::Response *m_response;
    util::IPEndPoint m_ipe;
    unsigned int m_access;
    boost::thread_specific_ptr<Server::Impl::SoapInfo> *m_endpoints;
    soap::Params m_args;
    soap::Parser m_soap_parser;
    xml::SaxParser m_parser;
    unsigned int m_action;

public:
    SoapReplier(Service *service, unsigned int action,
		util::http::Response *response,
		util::IPEndPoint ipe, unsigned int access,
		boost::thread_specific_ptr<SoapInfo> *endpoints);

    ~SoapReplier()
    {
	// End of body -- assemble reply
	unsigned int rc = ENOSYS;

	soap::Params result;
	
	SoapInfo *soap_info = new SoapInfo;
	soap_info->ipe = m_ipe;
	soap_info->access = m_access;
	m_endpoints->reset(soap_info);

	rc = m_service->OnAction(m_action, m_args, &result);

	if (rc)
	{
	    m_response->status_line = "HTTP/1.1 801 Error\r\n";

	    std::string body(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
		"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
		" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
		" <s:Body>"
		"  <s:Fault><faultcode>soap:Server</faultcode>"
		"  <faultstring>Service error ");
	    body += util::SPrintf("%u", rc);
	    body += "</faultstring></s:Fault></s:Body></s:Envelope>\r\n";
	
	    LOG(SOAP) << "Soap response is " << body << "\n";
	
	    m_response->body_source.reset(new util::StringStream(body));
	}
	else
	{
	    std::string body = soap::CreateBody(m_service->GetData(),
						m_action, true,
						m_service->GetServiceType(),
						result);
	
	    LOG(SOAP) << "Soap response is " << body << "\n";
	
	    m_response->body_source.reset(new util::StringStream(body));
	}
    }

    // Being a Stream
    unsigned GetStreamFlags() const { return WRITABLE; }
    unsigned Read(void *, size_t, size_t*) { return EPERM; }
    unsigned Write(const void *buffer, size_t len, size_t *pwrote)
    {
	unsigned int rc = m_parser.WriteAll(buffer, len);
	if (!rc)
	    *pwrote = len;
	return rc;
    }
};

Server::Impl::SoapReplier::SoapReplier(Service *service, unsigned int action,
				       util::http::Response *response,
				       util::IPEndPoint ipe, unsigned int access,
				       boost::thread_specific_ptr<SoapInfo> *endpoints)
    : m_service(service),
      m_response(response),
      m_ipe(ipe),
      m_access(access),
      m_endpoints(endpoints),
      m_soap_parser(service->GetData(), 
		    service->GetData()->action_args[action], &m_args),
      m_parser(&m_soap_parser),
      m_action(action)
{
}

bool Server::Impl::StreamForPath(const util::http::Request *rq, 
				 util::http::Response *rs)
{
    LOG(UPNP) << "Got " << rq->verb << " request for " << rq->path << "\n";

    const char *path = rq->path.c_str();
    const char *usn;

    if (rq->path == s_description_path)
    {
	LOG(UPNP) << "Serving description request\n";
	rs->body_source.reset(
	    new util::StringStream(Description(rq->local_ep.addr)));
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

		    m_subscriptions.push_back(Subscription(service, delivery));

		    rs->headers["SID"] = delivery; // why not?
		    rs->headers["TIMEOUT"] = "infinite";
		    rs->body_source.reset(new util::StringStream(""));
		}
	    }
	    else
	    {
		TRACE << "Can't find service " << usn << "\n";
	    }
	    return true;
	}
	else if (rq->verb == "UNSUBSCRIBE")
	{
	    LOG(UPNP) << "I'm being unsubscribed at\n" << rq->headers;

	    Service *service = FindService(usn);
	    if (service)
	    {
		std::string delivery = rq->GetHeader("SID");
		
		for (subscriptions_t::iterator i = m_subscriptions.begin();
		     i != m_subscriptions.end();
		     ++i)
		{
		    if (i->service == service
			&& i->delivery_url == delivery)
		    {
			m_subscriptions.erase(i);
			LOG(UPNP) << "Found and erased subscription\n";
			rs->body_source.reset(new util::StringStream(""));
			break;
		    }
		}
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
		std::string action_name = rq->GetHeader("SOAPACTION");
		if (!action_name.empty())
		{
		    action_name.erase(0,action_name.rfind('#')+1);
		    action_name.erase(action_name.rfind('\"'));
		    LOG(UPNP) << "Soap action: " << action_name << "\n";

		    const upnp::Data *data = svc->GetData();
		    unsigned int action = data->actions.Find(action_name.c_str());
		    if (action < data->actions.n)
		    {
			rs->content_type = "text/xml; charset=\"utf-8\"";
			rs->body_sink.reset(new SoapReplier(svc, action, rs,
							    rq->local_ep,
							    rq->access, 
							    &m_endpoints));
			return true;
		    }
		    else
		    {
			LOG(UPNP) << "Action " << action_name << " not found\n";
		    }
		}
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
