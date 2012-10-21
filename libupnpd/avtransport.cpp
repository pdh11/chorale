#include "config.h"
#include "avtransport.h"
#include "libutil/trace.h"
#include "liboutput/urlplayer.h"
#include "libutil/xmlescape.h"
#include "libutil/printf.h"
#include <stdio.h>

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
    case output::STOP:
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
	Fire(&upnp::AVTransportObserver::OnLastChange, change);
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
    Fire(&upnp::AVTransportObserver::OnLastChange, change);
}

void AVTransportImpl::OnTimeCode(unsigned int sec)
{
    m_timecodesec = sec;
}

static bool ParseTime(const std::string& target, unsigned int *pms)
{
    unsigned int ms;
    unsigned int h,m,s,f0,f1;
    if (sscanf(target.c_str(), "%u:%u:%u.%u/%u", &h, &m, &s, &f0, &f1) == 5)
    {
	if (f1 > 0 && f0 < f1)
	    ms = (f0 * 1000 / f1);
	else
	    ms = 0;
    }
    else if (sscanf(target.c_str(), "%u:%u:%u.%u", &h, &m, &s, &f0) == 4)
    {
	unsigned int digits = (unsigned int)(target.size()-target.rfind('.') - 1);
	while (digits > 3)
	{
	    f0 /= 10;
	    --digits;
	}
	while (digits < 3)
	{
	    f0 *= 10;
	    ++digits;
	}
	ms = f0;
    }
    else if (sscanf(target.c_str(), "%u:%u:%u", &h, &m, &s) == 3)
    {
	// OK
	ms = 0;
    }
    else
    {
	TRACE << "Don't like timecode '" << target << "'\n";
	return false;
    }
    
    *pms = h * 3600 * 1000
	+ m * 60 * 1000
	+ s * 1000
	+ ms;
    return true;
}

unsigned int AVTransportImpl::Seek(uint32_t, SeekMode unit, 
				   const std::string& target)
{
    if (unit != SEEKMODE_ABS_TIME)
	return 0;
    unsigned int ms;
    if (!ParseTime(target, &ms))
	return 0;
    return m_player->Seek(ms);
}

unsigned int AVTransportImpl::GetPositionInfo(uint32_t,
					      uint32_t *track,
					      std::string *track_duration,
					      std::string *track_meta_data,
					      std::string *track_uri,
					      std::string *rel_time,
					      std::string *abs_time,
					      int32_t *rel_count,
					      uint32_t *abs_count)
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
	std::string t = util::SPrintf("%02u:%02u:%02u", h, m, s);
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

#ifdef TEST

static const struct {
    const char *timecode;
    unsigned int ms;
} tests[] = {
    { "0:0:0",       0 },
    { "0:0:1",    1000 },
    { "0:1:0",   60000 },
    { "1:0:0", 3600000 },
    { "00:01:00.1",    60100 },
    { "00:01:00.01",   60010 },
    { "00:01:00.001",  60001 },
    { "00:01:00.0001", 60000 },
    { "00:01:00.1/25", 60040 },
    { "00:01:00.12/25", 60480 },
};

int main()
{
    unsigned int ms;
    for (unsigned int i=0; i<sizeof(tests)/sizeof(tests[0]); ++i)
    {
	bool ok = upnpd::ParseTime(tests[i].timecode, &ms);
	assert(ok);
	if (ms != tests[i].ms)
	{
	    TRACE << tests[i].timecode << " expected " << tests[i].ms
		  << " got " << ms << "\n";
	}
	assert(ms == tests[i].ms);
    }
    return 0;
}

#endif
