#include "media_server.h"
#include "libupnp/ssdp.h"

namespace upnpd {

MediaServer::MediaServer(mediadb::Database *db,
			 upnp::soap::InfoSource *info_source)
    : upnp::Device("urn:schemas-upnp-org:device:MediaServer:1"),
      m_contentdirectory(db, info_source),
      m_contentdirectoryserver(this,
			       upnp::s_service_id_content_directory,
			       "urn:schemas-upnp-org:service:ContentDirectory:1",
			       "/upnp/ContentDirectory.xml",
			       &m_contentdirectory),
      m_connection_manager(ConnectionManagerImpl::SERVER,
			   "http-get:*:*:*"),
      m_connection_manager_server(this,
				  upnp::s_service_id_connection_manager,
				  "urn:schemas-upnp-org:service:ConnectionManager:3",
				  "/upnp/ConnectionManager.xml",
				  &m_connection_manager)
{
}

} // namespace upnpd
