#include "media_renderer.h"
#include "libupnp/ssdp.h"

namespace upnpd {

MediaRenderer::MediaRenderer(output::URLPlayer *player)
    : upnp::Device(upnp::s_device_type_media_renderer),
      m_avtransport(player),
      m_avtransportserver(this,
			  upnp::s_service_id_av_transport,
			  "urn:schemas-upnp-org:service:AVTransport:2",
			  "/upnp/AVTransport.xml",
			  &m_avtransport),
      m_rcserver(this,
		 upnp::s_service_id_rendering_control,
		 "urn:schemas-upnp-org:service:RenderingControl:2",
		 "/upnp/RenderingControl.xml",
		 &m_rcstub),
      m_connection_manager(ConnectionManagerImpl::CLIENT,
			   "http-get:*:*:*"), ///< @todo Query player for info
      m_connection_manager_server(this,
				  upnp::s_service_id_connection_manager,
				  "urn:schemas-upnp-org:service:ConnectionManager:2",
				  "/upnp/ConnectionManager.xml",
				  &m_connection_manager)
{
}

} // namespace upnpd
