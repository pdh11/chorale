#include "media_server.h"

namespace upnpd {

MediaServer::MediaServer(mediadb::Database *db,
			 unsigned short port,
			 const std::string& resource)
    : upnp::Device("urn:schemas-upnp-org:device:MediaServer:1", resource),
      m_contentdirectory(db, port),
      m_contentdirectoryserver("urn:schemas-upnp-org:service:ContentDirectory:1",
			       "/upnp/ContentDirectory2.xml",
			       &m_contentdirectory)
{
    AddService("urn:upnp-org:serviceId:ContentDirectory", 
	       &m_contentdirectoryserver);
}

} // namespace upnpd
