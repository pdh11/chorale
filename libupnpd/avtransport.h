#ifndef UPNPD_AVTRANSPORT_H
#define UPNPD_AVTRANSPORT_H 1

#include "libupnp/AVTransport2.h"
#include "liboutput/urlplayer.h"

namespace upnpd {

/** Actual implementation of upnp::AVTransport2 base class in terms of an
 * output::URLPlayer.
 */
class AVTransportImpl: public upnp::AVTransport2, public output::URLObserver
{
    output::URLPlayer *m_player;
    output::PlayState m_state;
    unsigned int m_timecodesec;
    std::string m_url;
    std::string m_metadata;
    std::string m_next_url;
    std::string m_next_metadata;

    std::string GetStateChange();
    std::string GetURLChange();
    static const char *const sm_change_header;
    static const char *const sm_change_footer;

public:
    explicit AVTransportImpl(output::URLPlayer *player);
    ~AVTransportImpl();

    // Being an AVTransport2
    unsigned int SetAVTransportURI(uint32_t InstanceID,
				   const std::string& CurrentURI,
				   const std::string& CurrentURIMetaData);
    unsigned int SetNextAVTransportURI(uint32_t InstanceID,
				       const std::string& NextURI,
				       const std::string& NextURIMetaData);
    unsigned int Stop(uint32_t InstanceID);
    unsigned int Play(uint32_t InstanceID, TransportPlaySpeed Speed);
    unsigned int Pause(uint32_t InstanceID);
    unsigned int GetPositionInfo(uint32_t instance_id,
				 uint32_t *track,
				 std::string *track_duration,
				 std::string *track_meta_data,
				 std::string *track_uri,
				 std::string *rel_time,
				 std::string *abs_time,
				 int32_t *rel_count,
				 int32_t *abs_count);
    unsigned int GetLastChange(std::string*);

    // Being a URLObserver
    void OnPlayState(output::PlayState);
    void OnURL(const std::string&);
    void OnTimeCode(unsigned int);
};

} // namespace upnpd

#endif
