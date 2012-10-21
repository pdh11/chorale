#include "config.h"
#include "device.h"
#include "soap.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/string_stream.h"
#include "libutil/xmlescape.h"
#include <sstream>
#include <errno.h>
#include <openssl/md5.h>
#include <uuid/uuid.h>

#ifdef HAVE_UPNP

#include <upnp/upnp.h>

namespace upnp {

Service::Service(const char *type, const char *scpdurl, soap::Server *server)
    : m_type(type), m_scpdurl(scpdurl), m_server(server)
{
}

Device::Device(const char *type, const std::string& resource)
    : m_type(type),
      m_resource(resource)
{
}

void Device::AddService(const char *serviceID, Service *service)
{
    m_services[serviceID] = service;
}

void Device::AddEmbeddedDevice(Device *edev)
{
    m_devices.insert(edev);
}

Device *Device::FindByUDN(const char *udn)
{
    if (udn == ("uuid:" + m_uuid))
	return this;

    for (devices_t::const_iterator i = m_devices.begin();
	 i != m_devices.end();
	 ++i)
    {
	Device *d = (*i)->FindByUDN(udn);
	if (d)
	    return d;
    }

    TRACE << "can't find device '" << udn << "' uuid='uuid:" << m_uuid << "'\n";
    return NULL;
}

class DeviceManager::Impl: public util::LibUPnPUser
{
    UpnpDevice_Handle m_handle;

    static int StaticCallback(Upnp_EventType, void*, void*);

    Service *FindService(const char *udn, const char *service);
    static std::string MakeUUID(const std::string& resource);

    unsigned int m_device_index;

public:
    Impl();
    ~Impl();

    Device *m_device;
    unsigned int Init();

    std::string Description(Device*);

    // Being a LibUPnPUser
    int OnUPnPEvent(int, void *event);
};

DeviceManager::Impl::Impl()
    : m_handle(0),
      m_device_index(0)
{
}

DeviceManager::Impl::~Impl()
{
    if (m_handle)
	UpnpUnRegisterRootDevice(m_handle);
}

int DeviceManager::Impl::StaticCallback(Upnp_EventType et, void *ev,
					void *cookie)
{
    DeviceManager::Impl *impl = (DeviceManager::Impl*)cookie;
    return impl->OnUPnPEvent((int)et, ev);
}

Service *DeviceManager::Impl::FindService(const char *udn, const char *service)
{
    Device *d = m_device->FindByUDN(udn);
    if (!d)
	return NULL;

    Device::services_t::const_iterator i = d->m_services.find(service);
    if (i == d->m_services.end())
	return NULL;
    return i->second;
}

/** Construct the UUID for the UPnP Universal Device Name (UDN).
 *
 * In order to get the same UUID each time we run, we bake the IP address
 * and the path into the UUID using MD5.
 */
std::string DeviceManager::Impl::MakeUUID(const std::string& resource)
{
    union {
	unsigned char md5[MD5_DIGEST_LENGTH];
	uuid_t uuid;
    } u;

    std::string key = "chorale ";
    key += UpnpGetServerIpAddress();
    key += " " + resource;

    MD5((const unsigned char*)key.c_str(), key.length(), u.md5);

    char buf[40];
    uuid_unparse(u.uuid, buf);

    TRACE << "key='" << key << "' uuid=" << buf << "\n";

    return std::string(buf);
}

static const char *safe(const char *s)
{
    return s ? s : "NULL";
}

int DeviceManager::Impl::OnUPnPEvent(int et, void *event)
{
    switch (et)
    {
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
	soap::Params params;

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
		params[node->nodeName] = safe(childnode->nodeValue);

	    node = node->nextSibling;
	}

	/* Make the call */

	soap::Params out = service->GetServer()->OnAction(uar->ActionName,
							  params);

	/* Assemble the response */

	std::ostringstream ss;
	ss << "<u:" << uar->ActionName << "Response xmlns:u=\""
	   << service->GetType() << "\">";
	for (soap::Params::const_iterator i = out.begin();
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
	break;
    }

    default:
	TRACE << "unhandled device event " << et << "\n";
	break;
    }

    return UPNP_E_SUCCESS;
}

std::string DeviceManager::Impl::Description(Device *d)
{
    if (d->m_uuid.empty())
	d->m_uuid = MakeUUID(d->m_resource);

    std::ostringstream ss;

    /* Note that all these are case-sensitive */

    ss << "<device>"
       << "<deviceType>" << d->m_type << "</deviceType>"
       << "<friendlyName>" << "chorale" << "</friendlyName>"
       << "<manufacturer>" << "Peter Hartley" << "</manufacturer>"
       << "<manufacturerURL>http://utter.chaos.org.uk/~pdh/software/chorale/</manufacturerURL>"
       << "<modelDescription>chorale</modelDescription>"
       << "<modelName>chorale</modelName>"
       << "<modelNumber>chorale</modelNumber>"
       << "<UDN>uuid:" << d->m_uuid << "</UDN>"
       << "<presentationURL>/</presentationURL>"
       << "<iconList><icon><mimetype>image/png</mimetype>"
       << " <width>32</width><height>32</height><depth>24</depth>"
       << " <url>/upnp/icon.png</url>"
       << "</icon></iconList>"
       << "<serviceList>";

    for (Device::services_t::const_iterator i = d->m_services.begin();
	 i != d->m_services.end();
	 ++i)
    {
	ss << "<service>"
	   << "<serviceType>" << i->second->GetType() << "</serviceType>"
	   << "<serviceId>" << i->first << "</serviceId>"
	   << "<SCPDURL>" << i->second->GetSCPDUrl() << "</SCPDURL>"
	   << "<controlURL>/upnpcontrol" << m_device_index << "</controlURL>"
	   << "<eventSubURL>/upnpevent" << m_device_index << "</eventSubURL>"
	   << "</service>"
	    ;
    }
    ss << "</serviceList>";

    ++m_device_index;

    if (!d->m_devices.empty())
    {
	ss << "<deviceList>";

	for (Device::devices_t::const_iterator i = d->m_devices.begin();
	     i != d->m_devices.end();
	     ++i)
	    ss << Description(*i);

	ss << "</deviceList>";
    }

    ss << "</device>\n";

    return ss.str();
}

unsigned int DeviceManager::Impl::Init()
{
    std::string s =
	"<?xml version=\"1.0\"?><root xmlns=\"urn:schemas-upnp-org:device-1-0\"><specVersion><major>1</major><minor>0</minor></specVersion>"
	+ Description(m_device)
	     + "</root>";

    TRACE << "My device description is:\n" << s << "\n";

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

    TRACE << "UPnP device started\n";

    return 0;
}


        /* DeviceManager */


DeviceManager::DeviceManager()
    : m_impl(new Impl)
{
}

DeviceManager::~DeviceManager()
{
    delete m_impl;
}

unsigned int DeviceManager::SetRootDevice(Device *device)
{
    m_impl->m_device = device;
    return m_impl->Init();
}

}; // namespace upnp

#endif // HAVE_UPNP
