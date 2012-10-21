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
    unsigned int Play(uint32_t InstanceID, const std::string& Speed);
    unsigned int Pause(uint32_t InstanceID);
    unsigned int GetPositionInfo(uint32_t InstanceID, uint32_t *Track,
				 std::string *TrackDuration,
				 std::string *TrackMetaData,
				 std::string *TrackURI,
				 std::string *RelTime,
				 std::string *AbsTime,
				 int32_t *RelCount,
				 int32_t *AbsCount);

    // Being a URLObserver
    void OnPlayState(output::PlayState);
    void OnURL(const std::string&);
    void OnTimeCode(unsigned int);
};

} // namespace upnpd

#endif
