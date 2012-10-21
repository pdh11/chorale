#include "media_renderer.h"

namespace upnpd {

MediaRenderer::MediaRenderer(output::URLPlayer *player,
			     const std::string& resource)
    : m_avtransport(player),
      m_avtransportserver(&m_avtransport),
      m_avtransportservice("urn:schemas-upnp-org:service:AVTransport:1",
			   "/upnp/AVTransport.xml",
			   &m_avtransportserver),
      m_rcserver(&m_rcstub),
      m_rcservice("urn:schemas-upnp-org:service:RenderingControl:1",
		  "/upnp/RenderingControl.xml",
		  &m_rcserver),
      m_device("urn:schemas-upnp-org:device:MediaRenderer:1", resource)
{
    m_device.AddService("urn:upnp-org:serviceId:AVTransport", 
			&m_avtransportservice);
    m_device.AddService("urn:upnp-org:serviceId:RenderingControl", 
			&m_rcservice);
}

};
