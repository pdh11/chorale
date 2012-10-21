#include "cd.h"
#include "libimport/cd_content_factory.h"
#include "libupnpd/optical_drive.h"
#include <boost/lambda/construct.hpp>

CDService::CDService(util::hal::Context *hal)
    : m_drives(hal)
{
}

CDService::~CDService()
{
    std::for_each(m_factories.begin(), m_factories.end(), 
		  boost::lambda::delete_ptr());
    std::for_each(m_devices.begin(), m_devices.end(),
		  boost::lambda::delete_ptr());
}

unsigned int CDService::Init(util::WebServer *ws, const char *hostname,
			     upnp::Device **ppdev)
{
    m_drives.Refresh();

    unsigned int cdrom_index = 0;
    for (import::CDDrives::const_iterator i = m_drives.begin();
	 i != m_drives.end();
	 ++i)
    {
	import::CDDrivePtr cdp = i->second;
	import::CDContentFactory *fac
	    = new import::CDContentFactory(cdrom_index++);
	m_factories.push_back(fac);

	ws->AddContentFactory(fac->GetPrefix(), fac);
	upnpd::OpticalDriveDevice *cdromdevice
	    = new upnpd::OpticalDriveDevice(cdp, fac, ws->GetPort());
	m_devices.push_back(cdromdevice);

	cdromdevice->SetFriendlyName(cdp->GetName()
				     + " on " + std::string(hostname));
	    
	if (*ppdev)
	    (*ppdev)->AddEmbeddedDevice(cdromdevice);
	else
	    *ppdev = cdromdevice;
    }
    return 0;
}
