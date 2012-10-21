#include "media_renderer.h"

namespace upnpd {

MediaRenderer::MediaRenderer(output::URLPlayer *player,
			     const std::string& resource)
    : upnp::Device("urn:schemas-upnp-org:device:MediaRenderer:1", resource),
      m_avtransport(player),
      m_avtransportserver("urn:schemas-upnp-org:service:AVTransport:1",
			  "/upnp/AVTransport.xml",
			  &m_avtransport),
      m_rcserver("urn:schemas-upnp-org:service:RenderingControl:1",
		  "/upnp/RenderingControl.xml",
		  &m_rcstub)
{
    AddService("urn:upnp-org:serviceId:AVTransport", &m_avtransportserver);
    AddService("urn:upnp-org:serviceId:RenderingControl", &m_rcserver);
}

} // namespace upnpd
