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
    if (m_player)
	m_player->AddObserver(this);
}

AVTransportImpl::~AVTransportImpl()
{
    if (m_player)
	m_player->SetPlayState(output::STOP);
}

unsigned int AVTransportImpl::SetAVTransportURI(uint32_t, 
						const std::string& url,
						const std::string& metadata)
{
    TRACE << "SetURL(" << url << ")\n";
    m_url = url;
    m_metadata = metadata;
    return m_player->SetURL(url, metadata);
}

unsigned int AVTransportImpl::SetNextAVTransportURI(uint32_t,
						    const std::string& url,
						    const std::string& metadata)
{
    TRACE << "SetNextURL(" << url << ")\n";
    m_next_url = url;
    m_next_metadata = metadata;
    return m_player->SetNextURL(url, metadata);
}

unsigned int AVTransportImpl::Stop(uint32_t)
{
    return m_player->SetPlayState(output::STOP);
}

unsigned int AVTransportImpl::Play(uint32_t, TransportPlaySpeed)
{
    return m_player->SetPlayState(output::PLAY);
}

unsigned int AVTransportImpl::Pause(uint32_t)
{
    return m_player->SetPlayState(output::PAUSE);
}

std::string AVTransportImpl::GetStateChange()
{
    const char *sstate;
    switch (m_state)
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

    return std::string("<TransportState val=\"") + sstate + "\"/>";
}

void AVTransportImpl::OnPlayState(output::PlayState state)
{
    if (state != m_state)
    {
	m_state = state;
	std::string change = sm_change_header + GetStateChange()
	    + sm_change_footer;
	TRACE << "Emitting LastChange\n" << change << "\n";
	Fire(&upnp::AVTransport2Observer::OnLastChange, change);
    }
}

std::string AVTransportImpl::GetURLChange()
{
    return "<AVTransportURI val=\"" + util::XmlEscape(m_url) + "\"/>";
}

const char *const AVTransportImpl::sm_change_header =
    "<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT\">"
    "<InstanceID val=\"0\">";

const char *const AVTransportImpl::sm_change_footer =
	"</InstanceID>"
	"</Event>";

void AVTransportImpl::OnURL(const std::string& url)
{
    m_url = url;
    if (url == m_next_url)
	m_metadata = m_next_metadata;
    std::string change = sm_change_header + GetURLChange() + sm_change_footer;
    TRACE << "Emitting LastChange\n" << change << "\n";
    Fire(&upnp::AVTransport2Observer::OnLastChange, change);
}

void AVTransportImpl::OnTimeCode(unsigned int sec)
{
    m_timecodesec = sec;
}

unsigned int AVTransportImpl::GetPositionInfo(uint32_t,
					      uint32_t *track,
					      std::string *track_duration,
					      std::string *track_meta_data,
					      std::string *track_uri,
					      std::string *rel_time,
					      std::string *abs_time,
					      int32_t *rel_count,
					      int32_t *abs_count)
{
    if (track)
	*track = 1;
    if (track_duration)
	*track_duration = "NOT_IMPLEMENTED";
    if (track_uri)
	*track_uri = m_url;
    if (track_meta_data)
	*track_meta_data = m_metadata;

    if (rel_time || abs_time)
    {
	unsigned int h = m_timecodesec/3600;
	unsigned int m = (m_timecodesec/60) % 60;
	unsigned int s = m_timecodesec % 60;
	std::string t = (boost::format("%02u:%02u:%02u") % h % m % s).str();
	if (rel_time)
	    *rel_time = t;
	if (abs_time)
	    *abs_time = t;
    }

    if (rel_count)
	*rel_count = 1;
    if (abs_count)
	*abs_count = 1;
    
    return 0;
}

unsigned int AVTransportImpl::GetLastChange(std::string *change)
{
    *change = sm_change_header + GetURLChange() + GetStateChange()
	+ sm_change_footer;
    return 0;
}

} // namespace upnpd
