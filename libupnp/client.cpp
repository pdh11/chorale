#include "config.h"
#include "client.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/ssdp.h"
#include "libutil/xmlescape.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include <sstream>
#include <errno.h>

#ifdef HAVE_UPNP

#include <upnp/upnp.h>
#include <upnp/ixml.h>

namespace upnp {

class Client::Impl: public util::LibUPnPUser
{
    upnp::Description m_desc;

    typedef std::map<std::string, ClientConnection*> sidmap_t;
    sidmap_t m_sidmap;

    friend class Client;

public:
    const upnp::Description& GetDescription() { return m_desc; }

    void AddListener(ClientConnection *conn, const std::string& sid)
	{
	    m_sidmap[sid] = conn;
	}
    void RemoveListener(ClientConnection*);

    // Being a LibUPnPUser
    int OnUPnPEvent(int, void*);
};

int Client::Impl::OnUPnPEvent(int et, void *event)
{
//    TRACE << "UPnP callback (" << et << ")\n";

    switch (et)
    {
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

	sidmap_t::const_iterator it = m_sidmap.find(e->Sid);
	if (it == m_sidmap.end())
	    break;
	ClientConnection *conn = it->second;

	IXML_NodeList *nl = ixmlDocument_getElementsByTagName(e->ChangedVariables,
							      const_cast<char *>("e:property"));
	if (nl)
	{
	    for (unsigned int i=0; i<ixmlNodeList_length(nl); ++i)
	    {
		IXML_Node *node = ixmlNodeList_item(nl, i);
		if (node)
		{
		    IXML_Node *textnode = ixmlNode_getFirstChild(node);
		    if (textnode)
		    {
			const DOMString name = ixmlNode_getNodeName(textnode);
			IXML_Node *node2 = ixmlNode_getFirstChild(textnode);
			if (node2)
			{
			    const DOMString value = ixmlNode_getNodeValue(node2);
//			    TRACE << "var '" << name << "' val '" << value << "'\n";
			    conn->OnEvent(name, value);
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

Client::Client()
    : m_impl(NULL)
{
}

Client::~Client()
{
    delete m_impl;
}

unsigned int Client::Init(const std::string& descurl, const std::string& udn)
{
    m_impl = new Impl;

    unsigned int rc = m_impl->m_desc.Fetch(descurl, udn);
    if (rc != 0)
    {
	TRACE << "Can't Fetch()\n";
	return rc;
    }

    return 0;
}

const Description& Client::GetDescription() const
{
    return m_impl->m_desc;
}


/* ClientConnection */


struct ClientConnection::Impl
{
    Client::Impl *parent;
    ClientConnection *connection;
    const char *service;
    std::string control_url;
    std::string sid;
};

ClientConnection::ClientConnection()
    : m_impl(NULL)
{
}


ClientConnection::~ClientConnection()    
{
    delete m_impl;
}

static int SubscriptionCallback(Upnp_EventType et, void *event, void *cookie)
{
    ClientConnection *conn = (ClientConnection*)cookie;

    if (et == UPNP_EVENT_SUBSCRIBE_COMPLETE)
    {
	Upnp_Event_Subscribe *ues = (Upnp_Event_Subscribe*)event;
	if (ues->ErrCode == UPNP_E_SUCCESS)
	{
	    std::string sid = ues->Sid;
//	    TRACE << "Got sid '" << sid << "'\n";
	    conn->SetSid(sid);
	}
	else
	{
	    TRACE << "Subscription failed\n";
	}
    }

    return UPNP_E_SUCCESS;
}

unsigned int ClientConnection::Init(Client *parent, const char *service_id)
{
    if (!parent || !parent->m_impl || m_impl)
	return EINVAL;

    const upnp::Services& svc = parent->m_impl->GetDescription().GetServices();

    upnp::Services::const_iterator it = svc.find(service_id);
    if (it == svc.end())
    {
	TRACE << "No such service '" << service_id << "'\n";
	return ENOENT;
    }

    m_impl = new Impl;
    m_impl->parent = parent->m_impl;
    m_impl->connection = this;
    m_impl->control_url = it->second.control_url;
    m_impl->service = service_id;

    int rc = UpnpSubscribeAsync((UpnpClient_Handle)parent->m_impl->GetHandle(),
				it->second.event_url.c_str(), 7200u,
				SubscriptionCallback, this);

    if (rc)
    {
	TRACE << "USA returned " << rc << "\n";
	return EINVAL;
    }

    return 0;
}

unsigned int ClientConnection::GenaUInt(const std::string& s)
{
    return (unsigned int)strtoul(s.c_str(), NULL, 10);
}

bool ClientConnection::GenaBool(const std::string& s)
{
    return soap::ParseBool(s);
}

void ClientConnection::SetSid(const std::string& sid)
{
    assert(m_impl);
    assert(m_impl->parent);
    m_impl->parent->AddListener(this, sid);
}

unsigned int ClientConnection::SoapAction(const char *action_name,
					  soap::Inbound *result)
{
    soap::Outbound no_params;
    return SoapAction(action_name, no_params, result);
}

unsigned int ClientConnection::SoapAction(const char *action_name,
					  const soap::Outbound& in,
					  soap::Inbound *result)
{
    std::ostringstream os;
    os << "<u:" << action_name << " xmlns:u=\"" << m_impl->service
       << "\">";

    for (soap::Outbound::const_iterator i = in.begin(); i != in.end(); ++i)
	os << "<" << i->first << ">" << util::XmlEscape(i->second) << "</" << i->first << ">";

    os << "</u:" << action_name << ">";

//    TRACE << os.str() << "\n";

    IXML_Document *request = ixmlParseBuffer(const_cast<char *>(os.str().c_str()));
    if (!request)
    {
	TRACE << "Can't ixmlparsebuffer\n";
	return EINVAL;
    }

    IXML_Document *response = NULL;
//    TRACE << "handle = " << m_impl->GetHandle() << "\n";
    int rc2 = UpnpSendAction((UpnpClient_Handle)m_impl->parent->GetHandle(),
			     m_impl->control_url.c_str(),
			     m_impl->service,
			     m_impl->parent->GetDescription().GetUDN().c_str(),
			     request,
			     &response);
    if (rc2 != 0)
    {
	TRACE << "SOAP failed, rc2 = " << rc2 << "\n";
	return (unsigned)errno;
    }

//    if (response)
//    {
//	DOMString ds = ixmlPrintDocument(response);
//	TRACE << ds << "\n";
//	ixmlFreeDOMString(ds);
//    }
    ixmlDocument_free(request);

    if (result)
    {
	IXML_Node *child = ixmlNode_getFirstChild(&response->n);
	if (child)
	{
	    IXML_NodeList *nl = ixmlNode_getChildNodes(child);
	    
	    size_t resultcount = ixmlNodeList_length(nl);
//	TRACE << resultcount << " result(s)\n";

	    for (unsigned int i=0; i<resultcount; ++i)
	    {
		IXML_Node *node2 = ixmlNodeList_item(nl, i);
		if (node2)
		{
		    const DOMString ds = ixmlNode_getNodeName(node2);
		    if (ds)
		    {
			IXML_Node *textnode = ixmlNode_getFirstChild(node2);
			if (textnode)
			{
			    const DOMString ds2 = ixmlNode_getNodeValue(textnode);
			    if (ds2)
			    {
				//TRACE << ds << "=" << ds2 << "\n";
				result->Set(ds, ds2);
			    }
			    else
				TRACE << "no value\n";
			}
			//else
			//TRACE << "no textnode\n";
		    }
		    else
			TRACE << "no name\n";
		}
		else
		    TRACE << "no node\n";
	    }
	    
	    ixmlNodeList_free(nl);
	}
	else
	    TRACE << "no child\n";
    }

    ixmlDocument_free(response);

    return 0;
}

} // namespace upnp

#endif // HAVE_UPNP
