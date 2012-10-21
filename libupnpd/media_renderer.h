#ifndef LIBUPNPD_MEDIARENDERER_H
#define LIBUPNPD_MEDIARENDERER_H 1

#include "libupnp/AVTransport_server.h"
#include "libupnp/RenderingControl.h"
#include "libupnp/RenderingControl_server.h"
#include "libupnp/ConnectionManager_server.h"
#include "libupnp/device.h"
#include "avtransport.h"
#include "connection_manager.h"

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
    upnp::AVTransportServer m_avtransportserver;

    upnp::RenderingControl m_rcstub;
    upnp::RenderingControlServer m_rcserver;
    
    upnpd::ConnectionManagerImpl m_connection_manager;
    upnp::ConnectionManagerServer m_connection_manager_server;

public:
    explicit MediaRenderer(output::URLPlayer*);
};

} // namespace upnpd

#endif
