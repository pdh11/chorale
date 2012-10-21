#ifndef UPNPD_AVTRANSPORT_H
#define UPNPD_AVTRANSPORT_H 1

#include "libupnp/AVTransport2.h"

namespace output { class URLPlayer; };

namespace upnpd {

class AVTransportImpl: public upnp::AVTransport2
{
    output::URLPlayer *m_player;

public:
    explicit AVTransportImpl(output::URLPlayer *player);
    ~AVTransportImpl();

    // Being an AVTransport2

    unsigned int SetAVTransportURI(uint32_t InstanceID,
				   std::string CurrentURI,
				   std::string CurrentURIMetaData);
    unsigned int SetNextAVTransportURI(uint32_t InstanceID,
				       std::string NextURI,
				       std::string NextURIMetaData);
    unsigned int Stop(uint32_t InstanceID);
    unsigned int Play(uint32_t InstanceID, std::string Speed);
    unsigned int Pause(uint32_t InstanceID);
};

}; // namespace upnpd

#endif
