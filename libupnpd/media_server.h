#ifndef LIBUPNPD_MEDIASERVER_H
#define LIBUPNPD_MEDIASERVER_H 1

#include "libupnp/ContentDirectory2_server.h"
#include "libupnp/device.h"
#include "content_directory.h"

namespace mediadb { class Database; };

/** Classes implementing UPnP servers
 */
namespace upnpd {

class MediaServer
{
    upnpd::ContentDirectoryImpl m_contentdirectory;
    upnp::ContentDirectory2Server m_contentdirectoryserver;
    upnp::Service m_contentdirectoryservice;

    upnp::Device m_device;

public:
    MediaServer(mediadb::Database*, unsigned short port,
		const std::string& resource);

    upnp::Device *GetDevice() { return &m_device; }
};

}; // namespace upnpd

#endif
