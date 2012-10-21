#include "media_renderer.h"
#include "libupnp/ssdp.h"

namespace upnpd {

MediaRenderer::MediaRenderer(output::URLPlayer *player)
    : upnp::Device(upnp::s_device_type_media_renderer),
      m_avtransport(player),
      m_avtransportserver(this,
			  "urn:upnp-org:serviceId:AVTransport",
			  "urn:schemas-upnp-org:service:AVTransport:1",
			  "/upnp/AVTransport.xml",
			  &m_avtransport),
      m_rcserver(this,
		 "urn:upnp-org:serviceId:RenderingControl",
		 "urn:schemas-upnp-org:service:RenderingControl:1",
		  "/upnp/RenderingControl.xml",
		  &m_rcstub)
{
}

} // namespace upnpd
