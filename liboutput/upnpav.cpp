#include "config.h"
#include "upnpav.h"
#include "libutil/trace.h"
#include "libutil/xmlescape.h"
#include "libutil/xml.h"
#include "libutil/string_stream.h"
#include "libutil/poll.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include "libupnp/ssdp.h"
#include "libupnp/AVTransport2_client.h"
#include "libupnp/ConnectionManager2_client.h"
#include <errno.h>

namespace output {
namespace upnpav {

URLPlayer::URLPlayer(util::http::Client *client,
		     util::http::Server *server)
    : m_upnp(client, server),
      m_state(STOP),
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

    rc = m_avtransport.Init(&m_upnp, upnp::s_service_type_av_transport);
    if (rc != 0)
	return rc;
    m_avtransport.AddObserver(this);

    rc = m_connectionmanager.Init(&m_upnp, 
				  upnp::s_service_type_connection_manager);
    if (rc != 0)
	return rc;

    return 0;
}

unsigned int URLPlayer::OnTransportState(const std::string& state)
{
    TRACE << "Got TransportState " << state << "\n";
    output::PlayState ps = output::STOP;
    if (state == "PLAYING" || state == "TRANSITIONING")
	ps = output::PLAY;
    else if (state == "PAUSED_PLAYBACK")
	ps = output::PAUSE;
    
    Fire(&URLObserver::OnPlayState, ps);
			    
    if (ps == output::PLAY && m_state != output::PLAY)
	m_poller->Add(0, 500, this);
    else 
	if (ps != output::PLAY && m_state == output::PLAY)
	    m_poller->Remove(this);
    
    m_state = ps;
    return 0;
}

unsigned int URLPlayer::OnAVTransportURI(const std::string& url)
{
    TRACE << "Got URL " << url << "\n";
    Fire(&URLObserver::OnURL, url);
    return 0;
}

extern const char EVENT[] = "Event";
extern const char INSTANCEID[] = "InstanceID";
extern const char TRANSPORTSTATE[] = "TransportState";
extern const char AVTRANSPORTURI[] = "AVTransportURI";
extern const char VAL[] = "val";

typedef xml::Parser<
    xml::Tag<EVENT,
	     xml::Tag<INSTANCEID,
		      xml::Tag<TRANSPORTSTATE,
			       xml::Attribute<VAL, URLPlayer,
					      &URLPlayer::OnTransportState> >,
		      xml::Tag<AVTRANSPORTURI,
			       xml::Attribute<VAL, URLPlayer,
					      &URLPlayer::OnAVTransportURI> >
> > > LastChangeParser;
			       
void URLPlayer::OnLastChange(const std::string& value)
{
    LastChangeParser lcp;
    util::StreamPtr sp = util::StringStream::Create(value);
    lcp.Parse(sp, this);
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
	return m_avtransport.Play(0, upnp::AVTransport2::TRANSPORTPLAYSPEED_1);
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
	m_poller->Remove(this);
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
