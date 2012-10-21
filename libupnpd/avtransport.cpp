#include "config.h"
#include "avtransport.h"
#include "libutil/trace.h"
#include "liboutput/urlplayer.h"
#include "libutil/xmlescape.h"
#include <boost/format.hpp>

namespace upnpd {

AVTransportImpl::AVTransportImpl(output::URLPlayer *player)
    : m_player(player),
      m_state(output::STOP),
      m_timecodesec(0)
{
    m_player->AddObserver(this);
}

AVTransportImpl::~AVTransportImpl()
{
    m_player->SetPlayState(output::STOP);
}

unsigned int AVTransportImpl::SetAVTransportURI(uint32_t, 
						const std::string& CurrentURI,
						const std::string& metadata)
{
    TRACE << "SetURL(" << CurrentURI << ")\n";
    return m_player->SetURL(CurrentURI, metadata);
}

unsigned int AVTransportImpl::SetNextAVTransportURI(uint32_t,
						    const std::string& NextURI,
						    const std::string& metadata)
{
    TRACE << "SetNextURL(" << NextURI << ")\n";
    return m_player->SetNextURL(NextURI, metadata);
}

unsigned int AVTransportImpl::Stop(uint32_t)
{
    return m_player->SetPlayState(output::STOP);
}

unsigned int AVTransportImpl::Play(uint32_t, const std::string&)
{
    return m_player->SetPlayState(output::PLAY);
}

unsigned int AVTransportImpl::Pause(uint32_t)
{
    return m_player->SetPlayState(output::PAUSE);
}

void AVTransportImpl::OnPlayState(output::PlayState state)
{
    if (state != m_state)
    {
	m_state = state;
	const char *sstate;
	switch (state)
	{
	case output::PAUSE:
	    sstate = "PAUSED_PLAYBACK";
	    break;
	case output::PLAY:
	    sstate = "PLAYING";
	    break;
	default:
	    sstate = "STOPPED";
	    break;
	}

	std::string change =
	    "<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT\">"
	    "<InstanceID val=\"0\">"
	    "<TransportState val=\"";
	change += sstate;
	change += "\"/>"
	    "</InstanceID>"
	    "</Event>";
	TRACE << "Emitting LastChange\n" << change << "\n";
	Fire(&upnp::AVTransport2Observer::OnLastChange, change);
    }
}

void AVTransportImpl::OnURL(const std::string& url)
{
    std::string change =
	"<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT\">"
	"<InstanceID val=\"0\">"
	"<AVTransportURI val=\"" + util::XmlEscape(url) + "\"/>"
	"</InstanceID>"
	    "</Event>";
    TRACE << "Emitting LastChange\n" << change << "\n";
    Fire(&upnp::AVTransport2Observer::OnLastChange, change);
}

void AVTransportImpl::OnTimeCode(unsigned int sec)
{
    m_timecodesec = sec;
}

unsigned int AVTransportImpl::GetPositionInfo(uint32_t InstanceID,
					      uint32_t *Track,
					      std::string *TrackDuration,
					      std::string *TrackMetaData,
					      std::string *TrackURI,
					      std::string *RelTime,
					      std::string *AbsTime,
					      int32_t *RelCount,
					      int32_t *AbsCount)
{
    if (Track)
	*Track = 1;
    if (TrackDuration)
	*TrackDuration = "NOT_IMPLEMENTED";
//    if (TrackURI)
//	*TrackURI = m_url;

    if (RelTime)
    {
	unsigned int h = m_timecodesec/3600;
	unsigned int m = (m_timecodesec/60) % 60;
	unsigned int s = m_timecodesec % 60;
	*RelTime = (boost::format("%02u:%02u:%02u") % h % m % s).str();
    }
    
    return 0;
}

} // namespace upnpd
