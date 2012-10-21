#include "config.h"
#include "optical_drive.h"
#include "libimport/cd_content_factory.h"
#include "libutil/ssdp.h"
#include "libutil/trace.h"
#include <errno.h>
#include <boost/format.hpp>

#ifdef HAVE_UPNP

#include <upnp/upnp.h>

namespace upnpd {


        /* Device */


OpticalDriveDevice::OpticalDriveDevice(import::CDDrivePtr cd,
				       import::CDContentFactory *factory,
				       unsigned short port)
    : upnp::Device("urn:chorale-sf-net:dev:OpticalDrive:1", cd->GetName()),
      m_opticaldrive(cd, factory, port),
      m_opticaldriveserver(util::ssdp::s_uuid_opticaldrive,
			   "/upnp/OpticalDrive.xml", &m_opticaldrive)
{
    AddService("urn:chorale-sf-net:serviceId:OpticalDrive",
	       &m_opticaldriveserver);
}


        /* Service */


OpticalDriveImpl::OpticalDriveImpl(import::CDDrivePtr cd,
				   import::CDContentFactory *factory,
				   unsigned short port)
    : m_cd(cd),
      m_factory(factory),
      m_port(port),
      m_disc_present(false)
{
    m_cd->AddObserver(this);
}

OpticalDriveImpl::~OpticalDriveImpl()
{
    m_cd->RemoveObserver(this);
}

unsigned int OpticalDriveImpl::Eject()
{
    m_audiocd = NULL;
    return m_cd->Eject();
}

unsigned int OpticalDriveImpl::GetDiscInfo(uint8_t *track_count,
					   int32_t *total_sectors)
{
    if (m_audiocd == NULL)
    {
	unsigned int rc = m_cd->GetCD(&m_audiocd);
	if (rc != 0)
	    return rc;
	m_factory->SetAudioCD(m_audiocd);
    }

    *track_count = (uint8_t)m_audiocd->GetTrackCount();
    *total_sectors = m_audiocd->GetTotalSectors();
    return 0;
}

void OpticalDriveImpl::OnDiscPresent(bool whether)
{
    m_disc_present = whether;
    Fire(&upnp::OpticalDriveObserver::OnDiscPresent, whether);
    if (!whether)
    {
	m_audiocd = NULL;
	m_factory->ClearAudioCD();
    }
}

unsigned int OpticalDriveImpl::GetDiscPresent(bool *whether)
{
    *whether = m_disc_present;
    return 0;
}

unsigned int OpticalDriveImpl::GetTrackInfo(uint8_t track,
					    TrackType *type,
					    int32_t *first_sector,
					    int32_t *last_sector,
					    std::string *data_url)
{
    if (m_audiocd == NULL)
    {
	unsigned int rc = m_cd->GetCD(&m_audiocd);
	if (rc != 0)
	    return rc;
	m_factory->SetAudioCD(m_audiocd);
    }

    if (track >= m_audiocd->GetTrackCount())
	return ERANGE;

    /** @todo Determine IP address to use from socket (or SoapInfo structure)
     */
    std::string urlprefix;
    if (m_port)
    {
	urlprefix = (boost::format("http://%s:%u")
		     % UpnpGetServerIpAddress()
		     % m_port).str();
    }

    *type = TRACKTYPE_AUDIO;
    *first_sector = (m_audiocd->begin() + track)->first_sector;
    *last_sector = (m_audiocd->begin() + track)->last_sector;
    *data_url = (boost::format("%s%s%u.pcm") 
		 % urlprefix
		 % m_factory->GetPrefix()
		 % (unsigned int)track).str();
//    TRACE << "Track " << track << " url='" << *data_url << "'\n";
    return 0;
}

} // namespace upnpd

#endif // HAVE_UPNP
