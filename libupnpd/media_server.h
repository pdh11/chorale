#ifndef LIBUPNPD_MEDIASERVER_H
#define LIBUPNPD_MEDIASERVER_H 1

#include "libupnp/ContentDirectory_server.h"
#include "libupnp/ConnectionManager_server.h"
#include "libupnp/device.h"
#include "content_directory.h"
#include "connection_manager.h"

namespace mediadb { class Database; }
namespace upnp { namespace soap { class InfoSource; } }

namespace upnpd {

/** Wraps up the various components of a UPnP %MediaServer device ready to be
 * exported by upnp::Server.
 */
class MediaServer: public upnp::Device
{
    upnpd::ContentDirectoryImpl m_contentdirectory;
    upnp::ContentDirectoryServer m_contentdirectoryserver;
    
    upnpd::ConnectionManagerImpl m_connection_manager;
    upnp::ConnectionManagerServer m_connection_manager_server;

public:
    MediaServer(mediadb::Database*, upnp::soap::InfoSource*);
};

} // namespace upnpd

#endif
