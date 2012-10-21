#ifndef LIBUPNPD_MEDIARENDERER_H
#define LIBUPNPD_MEDIARENDERER_H 1

#include "libupnp/AVTransport2_server.h"
#include "libupnp/RenderingControl2.h"
#include "libupnp/RenderingControl2_server.h"
#include "libupnp/device.h"
#include "avtransport.h"

namespace output { class URLPlayer; }

namespace upnpd {

class MediaRenderer
{
    upnpd::AVTransportImpl m_avtransport;
    upnp::AVTransport2Server m_avtransportserver;
    upnp::Service m_avtransportservice;

    upnp::RenderingControl2 m_rcstub;
    upnp::RenderingControl2Server m_rcserver;
    upnp::Service m_rcservice;

    upnp::Device m_device;

public:
    MediaRenderer(output::URLPlayer*, const std::string& resource);

    upnp::Device *GetDevice() { return &m_device; }
};

}; // namespace upnpd

#endif
