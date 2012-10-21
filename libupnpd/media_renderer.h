#ifndef LIBUPNPD_MEDIARENDERER_H
#define LIBUPNPD_MEDIARENDERER_H 1

#include "libupnp/AVTransport2_server.h"
#include "libupnp/RenderingControl2.h"
#include "libupnp/RenderingControl2_server.h"
#include "libupnp/device.h"
#include "avtransport.h"

namespace output { class URLPlayer; }

/** Classes implementing UPnP servers
 */
namespace upnpd {

/** Wraps up the various components of a UPnP %MediaRenderer device ready to be
 * exported by upnp::DeviceManager.
 */
class MediaRenderer: public upnp::Device
{
    upnpd::AVTransportImpl m_avtransport;
    upnp::AVTransport2Server m_avtransportserver;

    upnp::RenderingControl2 m_rcstub;
    upnp::RenderingControl2Server m_rcserver;

public:
    MediaRenderer(output::URLPlayer*, const std::string& resource);
};

} // namespace upnpd

#endif
