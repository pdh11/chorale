#ifndef OUTPUT_UPNP_H
#define OUTPUT_UPNP_H

#include "urlplayer.h"
#include "libutil/bind.h"
#include "libutil/task.h"
#include "libutil/counted_pointer.h"
#include "libutil/observable.h"
#include "libupnp/AVTransport_client.h"
#include "libupnp/ConnectionManager_client.h"

namespace util { class Scheduler; }

namespace output {

/** Classes for playback of media by driving a remote UPnP A/V MediaRenderer.
 */
namespace upnpav {

/** Implementation of URLPlayer which drives a remote UPnP A/V MediaRenderer.
 */
class URLPlayer: public output::URLPlayer, public upnp::AVTransportObserver,
		 public upnp::AVTransportAsyncObserver,
		 public upnp::ConnectionManagerAsyncObserver,
		 public util::Observable<URLObserver>
{
public:
    typedef util::Callback1<unsigned int> InitCallback;

private:
    upnp::DeviceClient m_device_client;
    upnp::AVTransportClientAsync m_avtransport;
    upnp::ConnectionManagerClientAsync m_connectionmanager;
    PlayState m_state;
    util::Scheduler *m_poller;
    std::string m_friendly_name;
    bool m_supports_video;
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

    bool SupportsVideo() const { return m_supports_video; }

    // Being an URLPlayer
    unsigned int SetURL(const std::string& url,
                        const std::string& metadata) override;
    unsigned int SetNextURL(const std::string& url,
			    const std::string& metadata) override;
    unsigned int SetPlayState(PlayState) override;
    unsigned int Seek(unsigned int ms) override;

    // These are virtual in URLPlayer itself so must be defined explicitly
    void AddObserver(URLObserver*);
    void RemoveObserver(URLObserver*);
    
    // Being an AVTransport2Observer
    void OnLastChange(const std::string& value) override;
    
    // Being an AVTransport2AsyncObserver
    void OnGetPositionInfoDone(unsigned int rc,
			       uint32_t track,
			       const std::string& track_duration,
			       const std::string& track_meta_data,
			       const std::string& track_uri,
			       const std::string& rel_time,
			       const std::string& abs_time,
			       int32_t rel_count,
			       uint32_t abs_count) override;
    
    // Being a ConnectionManager2AsyncObserver
    void OnGetProtocolInfoDone(unsigned int rc,
			       const std::string& source,
			       const std::string& sink) override;

    // XML callbacks
    unsigned int OnTransportState(const std::string&);
    unsigned int OnAVTransportURI(const std::string&);
};

} // namespace upnpav

} // namespace output

#endif
