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
			       &m_contentdirectory)
{
}

} // namespace upnpd
