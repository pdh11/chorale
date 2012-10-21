#include "config.h"
#include "upnpav.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/ssdp.h"
#include "libutil/xmlescape.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include "libupnp/AVTransport2_client.h"
#include "libupnp/ConnectionManager2_client.h"
#include <errno.h>

#ifdef HAVE_UPNP

#include <upnp/ixml.h>

namespace output {
namespace upnpav {

URLPlayer::URLPlayer()
    : m_state(STOP),
      m_poller(NULL)
{
}

URLPlayer::~URLPlayer()
{
}

unsigned URLPlayer::Init(const std::string& url, const std::string& udn,
			 util::PollerInterface *poller)
{
    m_poller = poller;

    unsigned int rc = m_upnp.Init(url, udn);
    if (rc != 0)
	return rc;

    m_friendly_name = m_upnp.GetDescription().GetFriendlyName();

    rc = m_avtransport.Init(&m_upnp, util::ssdp::s_uuid_avtransport);
    if (rc != 0)
	return rc;
    m_avtransport.AddObserver(this);

    rc = m_connectionmanager.Init(&m_upnp, 
				  util::ssdp::s_uuid_connectionmanager);
    if (rc != 0)
	return rc;

    return 0;
}

void URLPlayer::OnLastChange(const std::string& value)
{
    IXML_Document *innerdoc = ixmlParseBuffer(const_cast<char *>(value.c_str()));
    if (innerdoc)
    {
	IXML_NodeList *nl2 
	    = ixmlDocument_getElementsByTagName(innerdoc,
						const_cast<char *>("AVTransportURI"));
	if (nl2)
	{
	    IXML_Node *node = ixmlNodeList_item(nl2, 0);
	    if (node)
	    {
		IXML_NamedNodeMap *nnm =
		    ixmlNode_getAttributes(node);
		if (nnm)
		{
		    IXML_Node *textnode = ixmlNamedNodeMap_getNamedItem(nnm,
							     const_cast<char *>("val"));
		    if (textnode)
		    {
			const DOMString ds = ixmlNode_getNodeValue(textnode);
			if (ds)
			{
			    std::string url = ds;
			    TRACE << "Got URL " << url << "\n";
			    Fire(&URLObserver::OnURL, url);
			}
		    }
		}
	    }
	    ixmlNodeList_free(nl2);
	}
	nl2 = ixmlDocument_getElementsByTagName(innerdoc,
						const_cast<char *>("TransportState"));
	if (nl2)
	{
	    IXML_Node *node = ixmlNodeList_item(nl2, 0);
	    if (node)
	    {
		IXML_NamedNodeMap *nnm =
		    ixmlNode_getAttributes(node);
		if (nnm)
		{
		    IXML_Node *textnode = ixmlNamedNodeMap_getNamedItem(nnm,
							     const_cast<char *>("val"));
		    if (textnode)
		    {
			const DOMString ds = ixmlNode_getNodeValue(textnode);
			if (ds)
			{
			    std::string state = ds;
			    TRACE << "Got TransportState " << state << "\n";
			    output::PlayState ps = output::STOP;
			    if (state == "PLAYING" || state == "TRANSITIONING")
				ps = output::PLAY;
			    else if (state == "PAUSED_PLAYBACK")
				ps = output::PAUSE;
			    
			    Fire(&URLObserver::OnPlayState, ps);
			    
			    if (ps == output::PLAY && m_state != output::PLAY)
				    m_poller->AddTimer(0, 500, this);
			    else 
				if (ps != output::PLAY && m_state == output::PLAY)
				    m_poller->RemoveTimer(this);

			    m_state = ps;
			}
		    }
		}
	    }
	    ixmlNodeList_free(nl2);
	}
	
	ixmlDocument_free(innerdoc);
    }
}

unsigned int URLPlayer::SetURL(const std::string& url,
			       const std::string& metadata)
{
    TRACE << "SetURL(" << url << ")\n";
    return m_avtransport.SetAVTransportURI(0, url, metadata);
}

unsigned int URLPlayer::SetNextURL(const std::string& url,
				   const std::string& metadata)
{
    TRACE << "SetNextURL(" << url << ")\n";
    return m_avtransport.SetNextAVTransportURI(0, url, metadata);
}

unsigned int URLPlayer::SetPlayState(PlayState state)
{
    switch (state)
    {
    case PLAY:
	TRACE << "Play\n";
	return m_avtransport.Play(0, "1");
    case STOP:
	TRACE << "Stop\n";
	return m_avtransport.Stop(0);
    case PAUSE:
	TRACE << "Pause\n";
	return m_avtransport.Pause(0);
    default:
	break;
    }

    return 0;
}

unsigned int URLPlayer::OnTimer()
{
    std::string tcstr;
    unsigned int rc = m_avtransport.GetPositionInfo(0, 
						    NULL,
						    NULL, NULL, NULL,
						    &tcstr, NULL,
						    NULL, NULL);
    if (rc != 0)
    {
	TRACE << "Can't GetPositionInfo\n";
	return rc;
    }

    unsigned int h, m, s;
    if (sscanf(tcstr.c_str(), "%u:%u:%u", &h, &m, &s) == 3)
    {
	Fire(&URLObserver::OnTimeCode, h*3600 + m*60 + s);
    }
    else
    {
	TRACE << "Don't like timecode string '" << tcstr << "'\n";
	m_poller->RemoveTimer(this);
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

} // namespace upnpav
} // namespace output

#endif // HAVE_UPNP
