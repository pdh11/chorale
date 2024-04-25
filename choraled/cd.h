#ifndef CHORALED_CD_H
#define CHORALED_CD_H

#include "config.h"
#include <list>
#include "libimport/cd_drives.h"

namespace upnpd { class OpticalDriveDevice; }
namespace import { class CDContentFactory; }
namespace util { namespace http { class Server; } }
namespace upnp { class Server; }

#define HAVE_CD HAVE_PARANOIA

class CDService
{
    import::CDDrives m_drives;
    std::list<import::CDContentFactory*> m_factories;
    std::list<upnpd::OpticalDriveDevice*> m_devices;

public:
    CDService();
    ~CDService();

    unsigned int Init(util::http::Server*, const char *hostname, 
		      upnp::Server*);
};

#endif
