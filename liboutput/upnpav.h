#ifndef OUTPUT_UPNP_H
#define OUTPUT_UPNP_H

#include "urlplayer.h"
#include "libutil/task.h"
#include "libutil/observable.h"
#include "libupnp/AVTransport2_client.h"
#include "libupnp/ConnectionManager2_client.h"

namespace util { class Scheduler; }

namespace output {

/** Classes for playback of media by driving a remote UPnP A/V MediaRenderer.
 */
namespace upnpav {

/** Implementation of URLPlayer which drives a remote UPnP A/V MediaRenderer.
 */
class URLPlayer: public output::URLPlayer, public upnp::AVTransport2Observer,
		 public util::Observable<URLObserver>
{
public:
    typedef util::Callback1<unsigned int> InitCallback;

private:
    upnp::DeviceClient m_device_client;
    upnp::AVTransport2Client m_avtransport; ///<@todo AVTransport2ClientAsync
    upnp::ConnectionManager2Client m_connectionmanager;
    PlayState m_state;
    util::Scheduler *m_poller;
    std::string m_friendly_name;
    InitCallback m_callback;

    class TimecodeTask;
    friend class TimecodeTask;
    typedef util::CountedPointer<TimecodeTask> TimecodeTaskPtr;
    TimecodeTaskPtr m_timecode_task;

    unsigned OnDeviceInitialised(unsigned int rc);
    unsigned OnTransportInitialised(unsigned int rc);
    unsigned OnInitialised(unsigned int rc);

public:
    URLPlayer(util::http::Client*, util::http::Server*,
	      util::Scheduler*);
    ~URLPlayer();

    unsigned Init(const std::string& url, const std::string& udn);

    unsigned Init(const std::string& url, const std::string& udn,
		  InitCallback callback);

    const std::string& GetFriendlyName() const { return m_friendly_name; }

    // Being an URLPlayer
    unsigned int SetURL(const std::string& url, const std::string& metadata);
    unsigned int SetNextURL(const std::string& url,
			    const std::string& metadata);
    unsigned int SetPlayState(PlayState);
    unsigned int Seek(unsigned int ms);

    // These are virtual in URLPlayer itself so must be defined explicitly
    void AddObserver(URLObserver*);
    void RemoveObserver(URLObserver*);
    
    // Being an AVTransport2Observer
    void OnLastChange(const std::string& value);

    // XML callbacks
    unsigned int OnTransportState(const std::string&);
    unsigned int OnAVTransportURI(const std::string&);
};

} // namespace upnpav

} // namespace output

#endif
