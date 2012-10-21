#include "media_server.h"
namespace upnpd {

MediaServer::MediaServer(mediadb::Database *db,
			 unsigned short port,
			 const std::string& resource)
    : m_contentdirectory(db, port),
      m_contentdirectoryserver(&m_contentdirectory),
      m_contentdirectoryservice("urn:schemas-upnp-org:service:ContentDirectory:1",
				"/upnp/ContentDirectory2.xml",
				&m_contentdirectoryserver),
      m_device("urn:schemas-upnp-org:device:MediaServer:1", resource)
{
    m_device.AddService("urn:upnp-org:serviceId:ContentDirectory", 
			&m_contentdirectoryservice);
}

};
