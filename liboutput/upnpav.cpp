#include "config.h"
#include "upnpav.h"
#include "libutil/trace.h"
#include "libutil/xml.h"
#include "libutil/string_stream.h"
#include "libutil/scheduler.h"
#include "libupnp/description.h"
#include "libupnp/ssdp.h"
#include <errno.h>
#include <boost/format.hpp>

namespace output {
namespace upnpav {

class URLPlayer::TimecodeTask: public util::Task
{
    URLPlayer *m_parent;
    util::Scheduler *m_scheduler;

public:
    TimecodeTask(URLPlayer *parent, util::Scheduler *scheduler)
	: m_parent(parent), m_scheduler(scheduler) {}

    unsigned OnTimer();
    unsigned Run() { return 0; }
};

URLPlayer::URLPlayer(util::http::Client *client,
		     util::http::Server *server,
		     util::Scheduler *poller)
    : m_device_client(client, server, poller),
      m_avtransport(&m_device_client, upnp::s_service_id_av_transport),
      m_connectionmanager(&m_device_client,
			  upnp::s_service_id_connection_manager),
      m_state(STOP),
      m_poller(poller),
      m_timecode_task(new TimecodeTask(this, poller))
{
}

URLPlayer::~URLPlayer()
{
    m_poller->Remove(m_timecode_task);
}

unsigned URLPlayer::Init(const std::string& url, const std::string& udn)
{
    unsigned int rc = m_device_client.Init(url, udn);
    if (rc)
	return rc;

    m_friendly_name = m_device_client.GetFriendlyName();

    rc = m_avtransport.Init();
    if (rc)
	return rc;
    m_avtransport.AddObserver(this);

    rc = m_connectionmanager.Init();
    if (rc)
	return rc;

    return 0;
}

unsigned URLPlayer::Init(const std::string& url, const std::string& udn,
			 InitCallback callback)
{
    m_callback = callback;
    return m_device_client.Init(url, udn,
				util::Bind1<unsigned int, URLPlayer, &URLPlayer::OnDeviceInitialised>(this));
}

unsigned URLPlayer::OnDeviceInitialised(unsigned int rc)
{
    if (!rc)
    {
	m_friendly_name = m_device_client.GetFriendlyName();
	rc = m_avtransport.Init(util::Bind1<unsigned int, URLPlayer, &URLPlayer::OnTransportInitialised>(this));
    }
    if (rc)
	m_callback(rc);
    return rc;
}

unsigned URLPlayer::OnTransportInitialised(unsigned int rc)
{
    if (!rc)
    {
	m_avtransport.AddObserver(this);
	rc = m_connectionmanager.Init(util::Bind1<unsigned int, URLPlayer, &URLPlayer::OnInitialised>(this));
	if (rc) // Ignore errors from connectionmanager
	{
	    rc = 0;
	    m_callback(0);
	}
    }
    if (rc)
	m_callback(rc);
    return rc;
}

unsigned URLPlayer::OnInitialised(unsigned int)
{
    return m_callback(0); // Ignore errors from connectionmanager
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
	m_poller->Wait(
	    util::Bind<TimecodeTask,&TimecodeTask::OnTimer>(m_timecode_task),
	    0, 500);
    else 
	if (ps != output::PLAY && m_state == output::PLAY)
	    m_poller->Remove(m_timecode_task);
    
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

unsigned int URLPlayer::Seek(unsigned int ms)
{
    unsigned int h = ms/3600000;
    unsigned int m = (ms/60000) % 60;
    unsigned int s = (ms/1000) % 60;
    ms = ms % 1000;
    std::string t = (boost::format("%02u:%02u:%02u.%03u")
		     % h % m % s % ms).str();
    return m_avtransport.Seek(0, upnp::AVTransport2::SEEKMODE_ABS_TIME, t);
}

void URLPlayer::AddObserver(URLObserver *obs)
{
    util::Observable<URLObserver>::AddObserver(obs);
}

void URLPlayer::RemoveObserver(URLObserver *obs)
{
    util::Observable<URLObserver>::RemoveObserver(obs);
}


/* TimecodeTask implementation */


unsigned int URLPlayer::TimecodeTask::OnTimer()
{
    std::string tcstr;
    unsigned int rc = m_parent->m_avtransport.GetPositionInfo(0, 
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
	m_parent->Fire(&URLObserver::OnTimeCode, h*3600 + m*60 + s);
    }
    else
    {
	TRACE << "Don't like timecode string '" << tcstr << "'\n";
	m_parent->m_poller->Remove(util::TaskPtr(this));
    }

    return 0;
}

} // namespace upnpav
} // namespace output
