#ifndef CHORALED_CD_H
#define CHORALED_CD_H

#include "config.h"
#include "libimport/cd_drives.h"

namespace upnpd { class OpticalDriveDevice; }
namespace import { class CDContentFactory; }
namespace util { namespace http { class Server; } }
namespace util { namespace hal { class Context; } }
namespace upnp { class Device; }
namespace upnp { class Server; }

#if (defined(HAVE_LIBCDIOP) || defined(HAVE_PARANOIA)) \
    && (defined(HAVE_HAL) || defined(WIN32))
#define HAVE_CD 1
#endif

class CDService
{
    import::CDDrives m_drives;
    std::list<import::CDContentFactory*> m_factories;
    std::list<upnpd::OpticalDriveDevice*> m_devices;

public:
    explicit CDService(util::hal::Context *hal);
    ~CDService();

    unsigned int Init(util::http::Server*, const char *hostname, 
		      upnp::Server*);
};

#endif
