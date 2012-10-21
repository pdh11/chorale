#ifndef UPNPD_OPTICAL_DRIVE_H
#define UPNPD_OPTICAL_DRIVE_H 1

#include "libupnp/OpticalDrive.h"
#include "libupnp/OpticalDrive_server.h"
#include "libimport/audio_cd.h"
#include "libimport/cd_drives.h"

namespace import { class CDContentFactory; }

namespace upnpd {

/** Actual implementation of upnp::OpticalDrive in terms of libimport.
 */
class OpticalDriveImpl: public upnp::OpticalDrive,
			public import::CDDriveObserver
{
    import::CDDrivePtr m_cd;
    import::AudioCDPtr m_audiocd;
    unsigned int m_index;
    import::CDContentFactory *m_factory;
    unsigned short m_port;
    bool m_disc_present;

public:
    OpticalDriveImpl(import::CDDrivePtr cd, import::CDContentFactory *factory,
		     unsigned short port);
    ~OpticalDriveImpl();
    
    // Being an OpticalDrive
    unsigned int Eject();
    unsigned int GetDiscInfo(uint8_t *track_count, int32_t *total_sectors);
    unsigned int GetTrackInfo(uint8_t track,
			      TrackType *type,
			      int32_t *first_sector,
			      int32_t *last_sector,
			      std::string *data_url);
    unsigned int GetDiscPresent(bool*);

    // Being a CDDriveObserver
    void OnDiscPresent(bool whether);
};

class OpticalDriveDevice: public upnp::Device
{
    upnpd::OpticalDriveImpl m_opticaldrive;
    upnp::OpticalDriveServer m_opticaldriveserver;

public:
    OpticalDriveDevice(import::CDDrivePtr cd, import::CDContentFactory*,
		       unsigned short port);
};

} // namespace upnpd

#endif