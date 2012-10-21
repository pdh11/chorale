#include "media_server.h"

namespace upnpd {

MediaServer::MediaServer(mediadb::Database *db,
			 upnp::soap::InfoSource *info_source)
    : upnp::Device("urn:schemas-upnp-org:device:MediaServer:1"),
      m_contentdirectory(db, info_source),
      m_contentdirectoryserver(this,
			       "urn:upnp-org:serviceId:ContentDirectory",
			       "urn:schemas-upnp-org:service:ContentDirectory:1",
			       "/upnp/ContentDirectory.xml",
			       &m_contentdirectory)
{
}

} // namespace upnpd
