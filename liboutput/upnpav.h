#ifndef OUTPUT_UPNP_H
#define OUTPUT_UPNP_H

#include "urlplayer.h"
#include "libutil/observable.h"
#include "libutil/poll.h"
#include "libupnp/AVTransport2_client.h"
#include "libupnp/ConnectionManager2_client.h"

namespace util { class PollerInterface; }

namespace output {

/** Classes for playback of media by driving a remote UPnP A/V MediaRenderer.
 */
namespace upnpav {

/** Implementation of URLPlayer which drives a remote UPnP A/V MediaRenderer.
 */
class URLPlayer: public output::URLPlayer, public upnp::AVTransport2Observer,
		 public util::Observable<URLObserver>, public util::Timed
{
    upnp::Client m_upnp;
    upnp::AVTransport2Client m_avtransport;
    upnp::ConnectionManager2Client m_connectionmanager;
    PlayState m_state;
    util::PollerInterface *m_poller;
    std::string m_friendly_name; 

public:
    URLPlayer(util::http::Client*, util::http::Server*);
    ~URLPlayer();

    unsigned Init(const std::string& url, const std::string& udn,
		  util::PollerInterface*);

    const std::string& GetFriendlyName() const { return m_friendly_name; }

    // Being an URLPlayer
    unsigned int SetURL(const std::string& url, const std::string& metadata);
    unsigned int SetNextURL(const std::string& url,
			    const std::string& metadata);
    unsigned int SetPlayState(PlayState);
    void AddObserver(URLObserver*);
    void RemoveObserver(URLObserver*);
    
    // Being an AVTransport2Observer
    void OnLastChange(const std::string& value);

    // XML callbacks
    unsigned int OnTransportState(const std::string&);
    unsigned int OnAVTransportURI(const std::string&);

    // Being a util::Timed
    unsigned int OnTimer();
};

} // namespace upnpav

} // namespace output

#endif
