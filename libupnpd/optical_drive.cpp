#include "config.h"
#include "optical_drive.h"
#include "libimport/cd_content_factory.h"
#include "libupnp/ssdp.h"
#include "libupnp/soap_info_source.h"
#include "libutil/trace.h"
#include <errno.h>
#include <boost/format.hpp>

namespace upnpd {


        /* Device */


OpticalDriveDevice::OpticalDriveDevice(import::CDDrivePtr cd,
				       import::CDContentFactory *factory,
				       upnp::soap::InfoSource *info_source)
    : upnp::Device(upnp::s_device_type_optical_drive),
      m_opticaldrive(cd, factory, info_source),
      m_opticaldriveserver(this,
			   "urn:chorale-sf-net:serviceId:OpticalDrive",
			   upnp::s_service_type_optical_drive,
			   "/upnp/OpticalDrive.xml", &m_opticaldrive)
{
}


        /* Service */


OpticalDriveImpl::OpticalDriveImpl(import::CDDrivePtr cd,
				   import::CDContentFactory *factory,
				   upnp::soap::InfoSource *info_source)
    : m_cd(cd),
      m_factory(factory),
      m_info_source(info_source),
      m_disc_present(!cd->SupportsDiscPresent()) // Pretend always there
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

    std::string urlprefix;
    if (m_info_source)
    {
	util::IPEndPoint ipe = m_info_source->GetCurrentEndPoint();
	urlprefix = "http://" + ipe.ToString();
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
