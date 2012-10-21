#include "upnp.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/ssdp.h"
#include "libutil/xmlescape.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include "libupnp/AVTransport2_client.h"
#include "libupnp/ConnectionManager2_client.h"
#include <errno.h>

namespace output {
namespace upnpav {

URLPlayer::URLPlayer()
    : m_state(STOP),
      m_avtransportsoap(NULL),
      m_avtransport(NULL),
      m_connectionmanagersoap(NULL),
      m_connectionmanager(NULL)
{
}

URLPlayer::~URLPlayer()
{
    delete m_connectionmanager;
    delete m_connectionmanagersoap;
    delete m_avtransport;
    delete m_avtransportsoap;
}

static int StaticCallback(Upnp_EventType et, void *event, void *cookie)
{
    URLPlayer *up = (URLPlayer*)cookie;
    return up->OnUPnPEvent(et, event);
}

unsigned URLPlayer::Init(const std::string& url)
{
//    m_description_url = url;

    upnp::Description desc;
    unsigned rc = desc.Fetch(url);
    if (rc != 0)
    {
	TRACE << "Can't Fetch()\n";
	return rc;
    }

    m_friendly_name = desc.GetFriendlyName();
    m_udn = desc.GetUDN();

    const upnp::Services& svc = desc.GetServices();

    upnp::Services::const_iterator it
	= svc.find(util::ssdp::s_uuid_connectionmanager);
    if (it != svc.end())
    {
	TRACE << "ConnectionManager is at " << it->second.control_url << "\n";
	m_connectionmanagersoap
	    = new upnp::soap::Connection(it->second.control_url, m_udn,
					 util::ssdp::s_uuid_connectionmanager);
	m_connectionmanager = new upnp::ConnectionManager2Client(m_connectionmanagersoap);
	TRACE << "ConnectionManager set up\n";
    }
    
    it = svc.find(util::ssdp::s_uuid_avtransport);
    if (it == svc.end())
    {
	TRACE << "No AVTransport service\n";
	return ENOENT;
    }
    
    TRACE << "AVTransport is at " << it->second.control_url << "\n";
    m_avtransportsoap = new upnp::soap::Connection(it->second.control_url,
						   m_udn,
						   util::ssdp::s_uuid_avtransport);
    m_avtransport = new upnp::AVTransport2Client(m_avtransportsoap);
    m_event_url = it->second.event_url;

    rc = UpnpSubscribeAsync(GetHandle(), m_event_url.c_str(), 7200,
			    &StaticCallback, this);

    TRACE << "USA returned " << rc << "\n";

    return 0;
}

int URLPlayer::OnUPnPEvent(int et, void *event)
{
//    TRACE << "UPnP callback (" << et << ")\n";

    switch (et)
    {
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    {
	Upnp_Event_Subscribe *ues = (Upnp_Event_Subscribe*)event;
	if (ues->ErrCode == UPNP_E_SUCCESS)
	{
	    m_sid = ues->Sid;
	    TRACE << "Got a sid '" << m_sid << "'\n";
	}
	break;
    }

    case UPNP_EVENT_RECEIVED:
    {

	/* An Event looks like this:

<?xml version="1.0"?>
<e:propertyset xmlns:e="urn:schemas-upnp-org:event-1-0">
<e:property>
<LastChange>&lt;Event xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/AVT/&quot;&gt;
&lt;InstanceID val=&quot;0&quot;&gt;
&lt;TransportState val=&quot;PLAYING&quot;/&gt;
&lt;/InstanceID&gt;
&lt;/Event&gt;</LastChange>
</e:property>
</e:propertyset>

        * or this:

<?xml version="1.0"?>
<e:propertyset xmlns:e="urn:schemas-upnp-org:event-1-0">
<e:property>
<LastChange>&lt;Event xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/AVT/&quot;&gt;
&lt;InstanceID val=&quot;0&quot;&gt;
&lt;AVTransportURI val=&quot;http://192.168.168.1:12078/content/1b200&quot;/&gt;
&lt;CurrentTrackMetaData val=&quot;&amp;lt;DIDL-Lite xmlns:dc=&amp;quot;http://purl.org/dc/elements/1.1/&amp;quot;
xmlns:upnp=&amp;quot;urn:schemas-upnp-org:metadata-1-0/upnp/&amp;quot;
xmlns=&amp;quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&amp;quot;&amp;gt;
&amp;lt;item&amp;gt;&amp;lt;upnp:class&amp;gt;object.item.audioItem.musicTrack&amp;lt;/upnp:class&amp;gt;&amp;lt;/item&amp;gt;&amp;lt;/DIDL-Lite&amp;gt;&quot;/&gt;
&lt;AVTransportURIMetaData val=&quot;&amp;lt;DIDL-Lite xmlns:dc=&amp;quot;http://purl.org/dc/elements/1.1/&amp;quot;
xmlns:upnp=&amp;quot;urn:schemas-upnp-org:metadata-1-0/upnp/&amp;quot;
xmlns=&amp;quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&amp;quot;&amp;gt;
&amp;lt;item&amp;gt;&amp;lt;upnp:class&amp;gt;object.item.audioItem.musicTrack&amp;lt;/upnp:class&amp;gt;&amp;lt;/item&amp;gt;&amp;lt;/DIDL-Lite&amp;gt;&quot;/&gt;
&lt;CurrentTrackDuration val=&quot;0:00:00&quot;/&gt;
&lt;NumberOfTracks val=&quot;1&quot;/&gt;
&lt;/InstanceID&gt;
&lt;/Event&gt;</LastChange>
</e:property>
</e:propertyset>

        * Yes, the contents of the LastChange tag is XML that's been
        * escaped in order to travel via XML, and the contents of the
        * CurrentTrackMetaData tag in that XML is more XML that's been
        * escaped a second time.
	*/

	struct Upnp_Event *e = (struct Upnp_Event*)event;

	if (e->Sid != m_sid)
	    break;

	IXML_NodeList *nl = ixmlDocument_getElementsByTagName(e->ChangedVariables,
							      "LastChange");
	if (nl)
	{
	    IXML_Node *node = ixmlNodeList_item(nl, 0);
	    if (node)
	    {
		IXML_Node *textnode = ixmlNode_getFirstChild(node);
		if (textnode)
		{
		    const DOMString ds = ixmlNode_getNodeValue(textnode);
		    if (ds)
		    {
//			TRACE << "LastChange: " << ds << "\n";
			IXML_Document *innerdoc = ixmlParseBuffer(ds);
			if (innerdoc)
			{
			    IXML_NodeList *nl2 
				= ixmlDocument_getElementsByTagName(innerdoc,
								    "AVTransportURI");
			    if (nl2)
			    {
				node = ixmlNodeList_item(nl2, 0);
				if (node)
				{
				    IXML_NamedNodeMap *nnm =
					ixmlNode_getAttributes(node);
				    if (nnm)
				    {
					textnode = ixmlNamedNodeMap_getNamedItem(nnm,
								    "val");
					if (textnode)
					{
					    ds = ixmlNode_getNodeValue(textnode);
					    if (ds)
					    {
						std::string url = ds;
						TRACE << "Got URL "
						      << url << "\n";
						Fire(&URLObserver::OnURL, url);
					    }
					}
				    }
				}
				ixmlNodeList_free(nl2);
			    }
			    nl2 = ixmlDocument_getElementsByTagName(innerdoc,
								   "TransportState");
			    if (nl2)
			    {
				node = ixmlNodeList_item(nl2, 0);
				if (node)
				{
				    IXML_NamedNodeMap *nnm =
					ixmlNode_getAttributes(node);
				    if (nnm)
				    {
					textnode = ixmlNamedNodeMap_getNamedItem(nnm,
										 "val");
					if (textnode)
					{
					    ds = ixmlNode_getNodeValue(textnode);
					    if (ds)
					    {
						std::string state = ds;
						TRACE << "Got TransportState "
						      << state << "\n";
						output::PlayState ps =
						    output::STOP;
						if (state == "PLAYING"
						    || state == "TRANSITIONING")
						    ps = output::PLAY;
						else if (state == "PAUSED_PLAYBACK")
						    ps = output::PAUSE;
						Fire(&URLObserver::OnPlayState,
						     ps);
					    }
					}
				    }
				}
				ixmlNodeList_free(nl2);
			    }

			    ixmlDocument_free(innerdoc);
			}
		    }
		}
	    }
	    ixmlNodeList_free(nl);
	}
	break;
    }
    default:
	break;
    }

    return UPNP_E_SUCCESS;
}

unsigned int URLPlayer::SetURL(const std::string& url,
			       const std::string& metadata)
{
    if (!m_avtransport)
	return ENOENT;
    return m_avtransport->SetAVTransportURI(0, url, metadata);
}

unsigned int URLPlayer::SetNextURL(const std::string& url,
				   const std::string& metadata)
{
    if (!m_avtransport)
	return ENOENT;
    return m_avtransport->SetNextAVTransportURI(0, url, metadata);
}

unsigned int URLPlayer::SetPlayState(PlayState state)
{
    switch (state)
    {
    case PLAY:
	TRACE << "Play\n";
	if (!m_avtransport)
	    return ENOENT;
	return m_avtransport->Play(0, "1");
    case STOP:
	TRACE << "Stop\n";
	if (!m_avtransport)
	    return ENOENT;
	return m_avtransport->Stop(0);
    case PAUSE:
	TRACE << "Pause\n";
	if (!m_avtransport)
	    return ENOENT;
	return m_avtransport->Pause(0);
    default:
	break;
    }

    return 0;
}

void URLPlayer::AddObserver(URLObserver *obs)
{
    util::Observable<URLObserver>::AddObserver(obs);
}

void URLPlayer::RemoveObserver(URLObserver *obs)
{
    util::Observable<URLObserver>::RemoveObserver(obs);
}

}; // namespace upnpav
}; // namespace output
