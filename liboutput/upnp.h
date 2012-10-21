#ifndef OUTPUT_UPNP_H
#define OUTPUT_UPNP_H

#include "urlplayer.h"
#include "libutil/upnp.h"
#include "libutil/observable.h"
#include <upnp/upnp.h>

namespace upnp { class AVTransport2; };
namespace upnp { class ConnectionManager2; };
namespace upnp { namespace soap { class Connection; }; };

namespace output {

/** Classes for playback of media by driving a remote UPnP A/V MediaRenderer.
 */
namespace upnpav {

class URLPlayer: public output::URLPlayer, public util::LibUPnPUser,
		 public util::Observable<URLObserver>
{
    std::string m_event_url;
    std::string m_udn;
    std::string m_friendly_name;
    PlayState m_state;
    upnp::soap::Connection *m_avtransportsoap;
    upnp::AVTransport2 *m_avtransport;
    upnp::soap::Connection *m_connectionmanagersoap;
    upnp::ConnectionManager2 *m_connectionmanager;
    std::string m_sid;

public:
    URLPlayer();
    ~URLPlayer();

    unsigned Init(const std::string& url);
    const std::string& GetFriendlyName() const { return m_friendly_name; }

    // Being an URLPlayer
    unsigned int SetURL(const std::string& url, const std::string& metadata);
    unsigned int SetNextURL(const std::string& url,
			    const std::string& metadata);
    unsigned int SetPlayState(PlayState);

    void AddObserver(URLObserver*);
    void RemoveObserver(URLObserver*);

    // Being a LibUPnPUser
    int OnUPnPEvent(int, void*);
};

}; // namespace upnpav

}; // namespace output

#endif
