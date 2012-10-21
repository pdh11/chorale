#ifndef LIBUPNPD_MEDIASERVER_H
#define LIBUPNPD_MEDIASERVER_H 1

#include "libupnp/ContentDirectory2_server.h"
#include "libupnp/device.h"
#include "content_directory.h"

namespace mediadb { class Database; }

namespace upnpd {

/** Wraps up the various components of a UPnP %MediaServer device ready to be
 * exported by upnp::Server.
 */
class MediaServer: public upnp::Device
{
    upnpd::ContentDirectoryImpl m_contentdirectory;
    upnp::ContentDirectory2Server m_contentdirectoryserver;

public:
    MediaServer(mediadb::Database*, unsigned short port,
		const std::string& resource);
};

} // namespace upnpd

#endif
