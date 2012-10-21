#include "config.h"
#include "server.h"
#include "device.h"
#include "soap.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/string_stream.h"
#include "libutil/xmlescape.h"
#include <sstream>
#include <errno.h>
#include <boost/format.hpp>

#if defined(HAVE_UPNP)

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

namespace upnp {

class Server::Impl: public util::LibUPnPUser
{
    Server *m_parent;
    UpnpDevice_Handle m_handle;

    static int StaticCallback(Upnp_EventType, void*, void*);

    Service *FindService(const char *udn, const char *service);
    static std::string MakeUUID(const std::string& resource);

    unsigned int m_device_index;
    std::string m_presentation_url;

public:
    Impl(Server*);
    ~Impl();

    Device *m_device;
    unsigned int Init(unsigned short port);

    std::string Description(Device*);
    void FireEvent(const std::string& udn, const char *service_id,
		   const char *variable, const std::string& value);

    // Being a LibUPnPUser
    int OnUPnPEvent(int, void *event);
};

Server::Impl::Impl(Server *parent)
    : m_parent(parent),
      m_handle(0),
      m_device_index(0)
{
}

Server::Impl::~Impl()
{
    if (m_handle)
	UpnpUnRegisterRootDevice(m_handle);
}

int Server::Impl::StaticCallback(Upnp_EventType et, void *ev,
					void *cookie)
{
    Server::Impl *impl = (Server::Impl*)cookie;
    return impl->OnUPnPEvent((int)et, ev);
}

Service *Server::Impl::FindService(const char *udn, const char *service)
{
    Device *d = m_device->FindByUDN(udn);
    if (!d)
	return NULL;

    Device::services_t::const_iterator i = d->m_services.find(service);
    if (i == d->m_services.end())
	return NULL;
    return i->second;
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
 * In order to get the same UUID each time we run, we bake the IP address
 * and the path into the UUID using MD5.
 */
std::string Server::Impl::MakeUUID(const std::string& resource)
{
    union {
	unsigned char ch[16];
	uint32_t ui[4];
    } u;

    strcpy((char*)u.ch, "chorale ");
    u.ui[2] = SimpleHash(UpnpGetServerIpAddress());
    u.ui[3] = SimpleHash(resource.c_str());

    char buf[40];
    sprintf(buf, "%08x-%04x-%04x-%04x-%04x%08x",
	    u.ui[0], u.ui[1] >> 16, u.ui[1] & 0xFFFF, u.ui[2] >> 16, 
	    u.ui[2] & 0xFFFF, u.ui[3]);

    return std::string(buf);
}

static const char *safe(const char *s)
{
    return s ? s : "NULL";
}

int Server::Impl::OnUPnPEvent(int et, void *event)
{
    switch (et)
    {
    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
    {
//	TRACE << "I'm being subscribed at\n";
	Upnp_Subscription_Request *usr = (Upnp_Subscription_Request*)event;
	Service *service = FindService(usr->UDN, usr->ServiceId);
	if (!service)
	{
	    TRACE << "Don't like udn '" << usr->UDN << "' svc '"
		  << usr->ServiceId << "'\n";
	    return UPNP_E_BAD_REQUEST;
	}
	soap::Outbound vars;
	service->GetEventedVariables(&vars);
	IXML_Document *propset = NULL;
	if (vars.begin() == vars.end())
	{
	    // The API requires at least one variable here
	    UpnpAddToPropertySet(&propset, "x-foo", "");
	}
	else
	{
	    for (soap::Outbound::const_iterator i = vars.begin();
		 i != vars.end();
		 ++i)
	    {
		UpnpAddToPropertySet(&propset, i->first,
				     i->second.c_str());
	    }
	}
	UpnpAcceptSubscriptionExt(m_handle, usr->UDN, usr->ServiceId,
				  propset, usr->Sid);
	ixmlDocument_free(propset);
	return UPNP_E_SUCCESS;
    }
    case UPNP_CONTROL_ACTION_REQUEST:
    {
	/* Stand by... for action */
	Upnp_Action_Request *uar = (Upnp_Action_Request*)event;
	Service *service = FindService(uar->DevUDN, uar->ServiceID);
	if (!service)
	{
	    TRACE << "Don't like udn '" << uar->DevUDN << "' svc '"
		  << uar->ServiceID << "'\n";
	    uar->ErrCode = UPNP_E_INVALID_ACTION;
	    return UPNP_E_INVALID_ACTION;
	}

//	TRACE << "Action " << uar->ActionName << "\n";

//	DOMString ds = ixmlPrintDocument(uar->ActionRequest);
//	TRACE << "Action doc:\n" << ds << "\n";
//	ixmlFreeDOMString(ds);

	/* Assemble arguments */
	soap::Inbound params;

	IXML_Node *node = uar->ActionRequest->n.firstChild;
	if (node)
	    node = node->firstChild;
	while (node)
	{
	    IXML_Node *childnode = node->firstChild;
//	    TRACE << node->nodeName << ", "
//		  << node->nodeValue << ", "
//		  << (childnode ? childnode->nodeName : "CHNULL") << ", "
//		  << (childnode ? childnode->nodeValue : "CHNULL") << "\n";
	    
	    if (node->nodeName && childnode)
		params.Set(node->nodeName, safe(childnode->nodeValue));

	    node = node->nextSibling;
	}

	/* Make the call */

	soap::Outbound out;
	unsigned int rc = service->OnAction(uar->ActionName, params, &out);

	/* Assemble the response */

	if (rc == 0)
	{
	    std::ostringstream ss;
	    ss << "<u:" << uar->ActionName << "Response xmlns:u=\""
	       << service->GetType() << "\">";
	    for (soap::Outbound::const_iterator i = out.begin();
		 i != out.end();
		 ++i)
	    {
		ss << "<" << i->first << ">" << util::XmlEscape(i->second)
		   << "</" << i->first << ">";
	    }
	    ss << "</u:" << uar->ActionName << "Response>";

	    std::string s = ss.str();
//	TRACE << "Reply doc:\n" << s << "\n";
	    uar->ActionResult = ixmlParseBuffer(const_cast<char *>(s.c_str()));
	    uar->ErrCode = UPNP_E_SUCCESS;
	}
	else
	{
	    uar->ErrCode = UPNP_SOAP_E_INVALID_ARGS;
	    return UPNP_SOAP_E_INVALID_ARGS;
	}
	break;
    }

    default:
//	TRACE << "unhandled device event " << et << "\n";
	break;
    }

    return UPNP_E_SUCCESS;
}

std::string Server::Impl::Description(Device *d)
{
    if (d->m_uuid.empty())
	d->m_uuid = MakeUUID(d->m_resource);

    d->m_server = m_parent;

    std::ostringstream ss;

    /* Note that all these are case-sensitive */

    ss << "<device>"
        "<deviceType>" << d->m_type << "</deviceType>"
        "<friendlyName>" << d->m_friendly_name << "</friendlyName>"
        "<manufacturer>" << "Peter Hartley" << "</manufacturer>"
	"<manufacturerURL>http://utter.chaos.org.uk/~pdh/software/chorale/</manufacturerURL>"
        "<modelDescription>chorale</modelDescription>"
        "<modelName>chorale</modelName>"
        "<modelNumber>chorale</modelNumber>"
        "<UDN>uuid:" << d->m_uuid << "</UDN>"
        "<presentationURL>" << m_presentation_url << "</presentationURL>"
        "<iconList>"
	"<icon><mimetype>image/png</mimetype>"
        " <width>32</width><height>32</height><depth>24</depth>"
        " <url>" << m_presentation_url << "layout/icon.png</url>"
        "</icon>"
	"<icon><mimetype>image/vnd.microsoft.icon</mimetype>"
        " <width>32</width><height>32</height><depth>24</depth>"
        " <url>" << m_presentation_url << "layout/icon.ico</url>"
        "</icon>"
	"<icon><mimetype>image/x-icon</mimetype>"
        " <width>32</width><height>32</height><depth>24</depth>"
        " <url>" << m_presentation_url << "layout/icon.ico</url>"
        "</icon>"
	"</iconList>"
        "<serviceList>";

    for (Device::services_t::const_iterator i = d->m_services.begin();
	 i != d->m_services.end();
	 ++i)
    {
	ss << "<service>"
	   << "<serviceType>" << i->second->GetType() << "</serviceType>"
	   << "<serviceId>" << i->first << "</serviceId>"
	   << "<SCPDURL>" << m_presentation_url << i->second->GetSCPDUrl() << "</SCPDURL>"
	   << "<controlURL>/upnpcontrol" << m_device_index << "</controlURL>"
	   << "<eventSubURL>/upnpevent" << m_device_index << "</eventSubURL>"
	   << "</service>"
	    ;
    }
    ss << "</serviceList>";

    ++m_device_index;

    if (!d->m_devices.empty())
    {
	ss << "<deviceList>\n";

	for (Device::devices_t::const_iterator i = d->m_devices.begin();
	     i != d->m_devices.end();
	     ++i)
	    ss << Description(*i);

	ss << "</deviceList>";
    }

    ss << "</device>\n";

    return ss.str();
}

unsigned int Server::Impl::Init(unsigned short port)
{
    m_presentation_url = (boost::format("http://%s:%u/")
			  % UpnpGetServerIpAddress()
			  % port).str();

    std::string s =
	"<?xml version=\"1.0\"?><root xmlns=\"urn:schemas-upnp-org:device-1-0\"><specVersion><major>1</major><minor>0</minor></specVersion>"
	+ Description(m_device)
	     + "</root>";

//    TRACE << "My device description is:\n" << s << "\n";

    int rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
				     s.c_str(),
				     s.length(),
				     1,
				     &StaticCallback,
				     this,
				     &m_handle);
    if (rc != UPNP_E_SUCCESS)
    {
	TRACE << "urrd2 failed " << rc << "\n";
	return ENOSYS;
    }
    rc = UpnpSendAdvertisement(m_handle, 100);
    if (rc != UPNP_E_SUCCESS)
    {
	TRACE << "usa failed " << rc << "\n";
	return ENOSYS;
    }

//    TRACE << "UPnP device started\n";

    return 0;
}

void Server::Impl::FireEvent(const std::string& udn, const char *service_id,
		       const char *variable, const std::string& value)
{
    IXML_Document *propset = NULL;
    
    UpnpAddToPropertySet(&propset, variable, value.c_str());
    int rc = UpnpNotifyExt(m_handle, udn.c_str(), service_id, propset);

    if (rc)
	TRACE << "UNE returned " << rc << "\n";

    ixmlDocument_free(propset);
}


        /* Server */


Server::Server()
    : m_impl(new Impl(this))
{
}

Server::~Server()
{
    delete m_impl;
}

unsigned int Server::Init(Device *device, unsigned short port)
{
    m_impl->m_device = device;
    return m_impl->Init(port);
}

void Server::FireEvent(const std::string& udn, const char *service_id,
		       const char *variable, const std::string& value)
{
    m_impl->FireEvent(udn, service_id, variable, value);
}

} // namespace upnp

#endif // HAVE_UPNP
