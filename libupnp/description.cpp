#include "description.h"
#include "libutil/string_stream.h"
#include "libutil/xml.h"
#include "libutil/trace.h"
#include "libutil/http.h"
#include "libutil/errors.h"

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

const char SERVICELIST[] = "serviceList";
const char SERVICE[] = "service";
const char SERVICETYPE[] = "serviceType";
const char SERVICEID[] = "serviceId";
const char CONTROLURL[] = "controlURL";
const char EVENTSUBURL[] = "eventSubURL";
const char SCPDURL[] = "SCPDURL";
const char UDN[] = "UDN";
const char FRIENDLYNAME[] = "friendlyName";
const char PRESENTATIONURL[] = "presentationURL";

const char ROOT[] = "root";
const char URLBASE[] = "URLBase";
const char DEVICE[] = "device";
const char DEVICELIST[] = "deviceList";
const char DEVICETYPE[] = "deviceType";

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
    util::StringStream sp(description);

    XMLDescription xd;
    DescriptionParser parser;
    unsigned rc = parser.Parse(&sp, &xd);
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

#ifdef TEST

# include "ssdp.h"

static const char sony[] =
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n"
"	<specVersion>\n"
"		<major>1</major>\n"
"		<minor>0</minor>\n"
"	</specVersion>\n"
"	<device>\n"
"		<deviceType>urn:schemas-upnp-org:device:MediaRenderer:1</deviceType>\n"
"		<friendlyName>BRAVIA KDL-40W5500     </friendlyName>\n"
"		<manufacturer>Sony Corporation</manufacturer>\n"
"		<manufacturerURL>http://www.sony.net/</manufacturerURL>\n"
"		<modelName>KDL-40W5500     </modelName>\n"
"		<UDN>uuid:00000000-0000-1010-8000-0024BE23675E</UDN>\n"
"		<dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">DMR-1.50</dlna:X_DLNADOC>\n"
"		<iconList>\n"
"			<icon>\n"
"				<mimetype>image/png</mimetype>\n"
"				<width>32</width>\n"
"				<height>32</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_32x32x24.png</url>\n"
"			</icon>\n"
"			<icon>\n"
"				<mimetype>image/png</mimetype>\n"
"				<width>48</width>\n"
"				<height>48</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_48x48x24.png</url>\n"
"			</icon>\n"
"			<icon>\n"
"				<mimetype>image/png</mimetype>\n"
"				<width>60</width>\n"
"				<height>60</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_60x60x24.png</url>\n"
"			</icon>\n"
"			<icon>\n"
"				<mimetype>image/png</mimetype>\n"
"				<width>120</width>\n"
"				<height>120</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_120x120x24.png</url>\n"
"			</icon>\n"
"			<icon>\n"
"				<mimetype>image/jpeg</mimetype>\n"
"				<width>32</width>\n"
"				<height>32</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_32x32x24.jpg</url>\n"
"			</icon>\n"
"			<icon>\n"
"				<mimetype>image/jpeg</mimetype>\n"
"				<width>48</width>\n"
"				<height>48</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_48x48x24.jpg</url>\n"
"			</icon>\n"
"			<icon>\n"
"				<mimetype>image/jpeg</mimetype>\n"
"				<width>60</width>\n"
"				<height>60</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_60x60x24.jpg</url>\n"
"			</icon>\n"
"			<icon>\n"
"				<mimetype>image/jpeg</mimetype>\n"
"				<width>120</width>\n"
"				<height>120</height>\n"
"				<depth>24</depth>\n"
"				<url>/MediaRenderer_120x120x24.jpg</url>\n"
"			</icon>\n"
"		</iconList>\n"
"		<serviceList>\n"
"			<service>\n"
"				<serviceType>urn:schemas-upnp-org:service:RenderingControl:1</serviceType>\n"
"				<serviceId>urn:upnp-org:serviceId:RenderingControl</serviceId>\n"
"				<SCPDURL>/RenderingControlSCPD.xml</SCPDURL>\n"
"				<controlURL>/upnp/control/RenderingControl</controlURL>\n"
"				<eventSubURL>/upnp/event/RenderingControl</eventSubURL>\n"
"			</service>\n"
"			<service>\n"
"				<serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>\n"
"				<serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>\n"
"				<SCPDURL>/ConnectionManagerSCPD.xml</SCPDURL>\n"
"				<controlURL>/upnp/control/ConnectionManager</controlURL>\n"
"				<eventSubURL>/upnp/event/ConnectionManager</eventSubURL>\n"
"			</service>\n"
"			<service>\n"
"				<serviceType>urn:schemas-upnp-org:service:AVTransport:1</serviceType>\n"
"				<serviceId>urn:upnp-org:serviceId:AVTransport</serviceId>\n"
"				<SCPDURL>/AVTransportSCPD.xml</SCPDURL>\n"
"				<controlURL>/upnp/control/AVTransport</controlURL>\n"
"				<eventSubURL>/upnp/event/AVTransport</eventSubURL>\n"
"			</service>\n"
"		</serviceList>\n"
"		<av:X_MaxBGMCount xmlns:av=\"urn:schemas-sony-com:av\">64</av:X_MaxBGMCount>\n"
"		<av:X_StandardDMR xmlns:av=\"urn:schemas-sony-com:av\">1.0</av:X_StandardDMR>\n"
"		<av:X_IRCCCodeList xmlns:av=\"urn:schemas-sony-com:av\">\n"
"			<av:X_IRCCCode command=\"Power\">AAAAAQAAAAEAAAAVAw==</av:X_IRCCCode>\n"
"			<av:X_IRCCCode command=\"Power ON\">AAAAAQAAAAEAAAAuAw==</av:X_IRCCCode>\n"
"			<av:X_IRCCCode command=\"Power OFF\">AAAAAQAAAAEAAAAvAw==</av:X_IRCCCode>\n"
"		</av:X_IRCCCodeList>\n"
"	</device>\n"
"</root>";

int main()
{
    upnp::Description d;

    unsigned rc = d.Parse(sony, "http://sony", 
			  "uuid:00000000-0000-1010-8000-0024BE23675E");
    assert(rc == 0);

    assert(d.GetFriendlyName() == "BRAVIA KDL-40W5500     ");

    const upnp::Services& sm = d.GetServices();

    assert(sm.size() == 3);

    assert(sm.find(upnp::s_service_id_av_transport) != sm.end());
    const upnp::ServiceData& s = sm.find(upnp::s_service_id_av_transport)->second;
    assert(s.type == upnp::s_service_type_av_transport);
    assert(s.id == upnp::s_service_id_av_transport);
    assert(s.control_url == "http://sony/upnp/control/AVTransport");

    return 0;
}

#endif // TEST
