#include "description.h"
#include "libutil/string_stream.h"
#include "libutil/xml.h"
#include "libutil/trace.h"
#include "libutil/http_fetcher.h"

namespace upnp {

struct XMLDevice
{
    std::string udn;
    std::string friendly_name;
    std::string presentation_url;
    std::list<ServiceData> services;
    std::list<XMLDevice> embedded_devices;
};

struct XMLDescription
{
    std::string url_base;
    XMLDevice root_device;
};

extern const char SERVICELIST[] = "serviceList";
extern const char SERVICE[] = "service";
extern const char SERVICETYPE[] = "serviceType";
extern const char SERVICEID[] = "serviceId";
extern const char CONTROLURL[] = "controlURL";
extern const char EVENTSUBURL[] = "eventSubURL";
extern const char SCPDURL[] = "SCPDURL";
extern const char UDN[] = "UDN";
extern const char FRIENDLYNAME[] = "friendlyName";
extern const char PRESENTATIONURL[] = "presentationURL";

extern const char ROOT[] = "root";
extern const char URLBASE[] = "URLBase";
extern const char DEVICE[] = "device";
extern const char DEVICELIST[] = "deviceList";
extern const char DEVICETYPE[] = "deviceType";

typedef xml::Tag<SERVICELIST,
		 xml::List<SERVICE, ServiceData,
			   XMLDevice, &XMLDevice::services,
			   xml::TagMember<SERVICETYPE, ServiceData,
					  &ServiceData::type>,
			   xml::TagMember<SERVICEID, ServiceData,
					  &ServiceData::id>,
			   xml::TagMember<CONTROLURL, ServiceData,
					  &ServiceData::control_url>,
			   xml::TagMember<EVENTSUBURL, ServiceData,
					  &ServiceData::event_url>,
			   xml::TagMember<SCPDURL, ServiceData,
					  &ServiceData::scpd_url>
> > ServiceListParser;

typedef xml::Parser<
    xml::Tag<ROOT,
	     xml::TagMember<URLBASE, XMLDescription,
			    &XMLDescription::url_base>,
	     xml::Structure<DEVICE, XMLDevice,
			    XMLDescription, &XMLDescription::root_device,
			    xml::TagMember<FRIENDLYNAME, XMLDevice,
					   &XMLDevice::friendly_name>,
			    xml::TagMember<UDN, XMLDevice,
					   &XMLDevice::udn>,
			    xml::TagMember<PRESENTATIONURL, XMLDevice,
					   &XMLDevice::presentation_url>,
			    ServiceListParser,
			    xml::Tag<DEVICELIST,
				     xml::List<DEVICE, XMLDevice,
					       XMLDevice, &XMLDevice::embedded_devices,
					       xml::TagMember<FRIENDLYNAME, XMLDevice,
							      &XMLDevice::friendly_name>,
					       xml::TagMember<UDN, XMLDevice,
							      &XMLDevice::udn>,
					       xml::TagMember<PRESENTATIONURL, XMLDevice,
							      &XMLDevice::presentation_url>,
					       ServiceListParser
> > > > > DescriptionParser;

unsigned Description::Parse(const std::string& description,
			    const std::string& url,
			    const std::string& udn)
{
    util::SeekableStreamPtr sp = util::StringStream::Create(description);

    XMLDescription xd;
    DescriptionParser parser;
    unsigned rc = parser.Parse(sp, &xd);
    if (rc)
    {
	TRACE << "Can't parse description\n";
	return rc;
    }

    const XMLDevice *d = &xd.root_device;
    if (d->udn != udn)
    {
	for (std::list<XMLDevice>::const_iterator i = d->embedded_devices.begin();
	     i != d->embedded_devices.end();
	     ++i)
	{
	    if (i->udn == udn)
	    {
		d = &(*i);
		break;
	    }
	}
	if (d->udn != udn)
	{
	    TRACE << "UDN not found in description\n";
	    return ENOENT;
	}
    }

    m_udn = udn;
    m_friendly_name = d->friendly_name;
    std::string base_url = xd.url_base.empty() ? url : xd.url_base;
    if (!d->presentation_url.empty())
	m_presentation_url = util::http::ResolveURL(base_url,
						    d->presentation_url);

    for (std::list<ServiceData>::const_iterator i = d->services.begin();
	 i != d->services.end();
	 ++i)
    {
	ServiceData sd = *i;
	sd.control_url = util::http::ResolveURL(base_url, sd.control_url);
	sd.event_url = util::http::ResolveURL(base_url, sd.event_url);
	sd.scpd_url = util::http::ResolveURL(base_url, sd.scpd_url);
	m_services[sd.id] = sd;
    }

    return 0;
}

} // namespace upnp
