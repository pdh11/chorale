#include "audio_cd.h"
#include "libutil/trace.h"
#include <cdio/cdio.h>
#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include <cdio/paranoia.h>
#include <errno.h>

namespace import {

unsigned AudioCD::Create(CDDrivePtr drive, AudioCDPtr *pcd)
{
    *pcd = AudioCDPtr(NULL);
    
    cdrom_drive_t *cdt =  cdio_cddap_identify(drive->GetDevice().c_str(),
					      1, NULL);
    if (!cdt)
    {
	TRACE << "Can't identify CD drive\n";
	return EINVAL;
    }

    int rc = cdio_cddap_open(cdt);
    if (rc<0)
    {
	TRACE << "Can't open CD drive\n";
	return errno;
    }

    unsigned int total = cdio_cddap_tracks(cdt);

    if (total == (track_t)-1)
    {
	cdio_cddap_close(cdt);
	TRACE << "No audio tracks\n";
	return ENOENT;
    }

    toc_t toc;

    unsigned total_sectors = 0;

    for (unsigned int i=0; i<total; ++i)
    {
	if (cdio_cddap_track_audiop(cdt, i+1))
	{
	    TocEntry te;
	    te.firstsector = cdio_cddap_track_firstsector(cdt, i+1);
	    te.lastsector  = cdio_cddap_track_lastsector(cdt, i+1);
//	    TRACE << "Track " << i+1 << " " << te.firstsector
//		  << ".." << te.lastsector << "\n";
	    
	    toc.push_back(te);

	    total_sectors += te.lastsector - te.firstsector + 1;
	}
	else
	{
	    TRACE << "Track " << i+1 << " not audio\n";
	}
    }

    if (toc.empty())
    {
	cdio_cddap_close(cdt);
	TRACE << "No audio tracks\n";
	return ENOENT;
    }

    AudioCDPtr cd = AudioCDPtr(new AudioCD());
    cd->m_toc = toc;
    cd->m_cdt = cdt;
    cd->m_device_name = drive->GetDevice();
    cd->m_total_sectors = total_sectors;
    *pcd = cd;
    return 0;
}

AudioCD::~AudioCD()
{
    if (m_cdt)
	cdio_cddap_close((cdrom_drive_t*)m_cdt);
//    TRACE << "~AudioCD\n";
}

}; // namespace import
