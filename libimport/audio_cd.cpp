#include "audio_cd.h"
#include "config.h"
#include "libutil/stream.h"
#include "libutil/trace.h"
#include "libutil/counted_pointer.h"
#include <errno.h>
#include <string.h>

// Oh joy, not C++-compatible
extern "C" {
//#define private c_private
#include <cdda_interface.h>
#include <cdda_paranoia.h>
//#undef private
}
typedef int paranoia_cb_mode_t;
typedef unsigned int track_t;

namespace import {

unsigned LocalAudioCD::Create(const std::string& device, AudioCDPtr *pcd)
{
    *pcd = AudioCDPtr(NULL);

    TRACE << "Calling identify\n";

    cdrom_drive *cdt =  cdda_identify(device.c_str(), CDDA_MESSAGE_FORGETIT, 
				      NULL);

    if (!cdt)
    {
	TRACE << "Can't identify CD drive\n";
	return EINVAL;
    }

    TRACE << "Identified\n";

    int rc = cdda_open(cdt);
    if (rc<0)
    {
	TRACE << "Can't open CD drive\n";
	return (unsigned)errno;
    }

    TRACE << "Opened\n";

    unsigned int total = (unsigned int)cdda_tracks(cdt);

    TRACE << "Got tracks\n";

    if (total == (track_t)-1)
    {
	cdda_close(cdt);
	TRACE << "No audio tracks\n";
	return ENOENT;
    }

    toc_t toc;

    unsigned total_sectors = 0;

    for (track_t i=1; i<=total; ++i) // Peculiar 1-based numbering
    {
	if (cdda_track_audiop(cdt, i))
	{
	    TocEntry te;
	    te.first_sector = (int)cdda_track_firstsector(cdt, i);
	    te.last_sector  = (int)cdda_track_lastsector(cdt, i);
	    TRACE << "Track " << i << " " << te.first_sector
		  << ".." << te.last_sector << "\n";
	    
	    toc.push_back(te);

	    total_sectors += (unsigned)(te.last_sector - te.first_sector + 1);
	}
	else
	{
//	    TRACE << "Track " << i+1 << " not audio\n";
	}
    }

    if (toc.empty())
    {
	cdda_close(cdt);
	TRACE << "No audio tracks\n";
	return ENOENT;
    }

    TRACE << "Opened with " << toc.size() << " tracks\n";

    LocalAudioCD *cd = new LocalAudioCD();
    cd->m_toc = toc;
    cd->m_cdt = cdt;
    cd->m_total_sectors = total_sectors;
    *pcd = AudioCDPtr(cd);
    return 0;
}

LocalAudioCD::~LocalAudioCD()
{
    if (m_cdt)
	cdda_close((cdrom_drive*)m_cdt);
//    TRACE << "~LocalAudioCD\n";
}

class ParanoiaStream: public util::SeekableStream
{
    cdrom_drive *m_drive;
    cdrom_paranoia *m_paranoia;
    int m_first_sector;
    int m_last_sector;
    int m_sector;
    const char *m_data;
    unsigned int m_skip;

    /** Neither cdparanoia nor libcdio lets us know which instance the
     * callback relates to. So we use C++11 thread-local storage
     * (there's at most one call to paranoia_read *per thread* active
     * at any one time).
     */
    thread_local static cdrom_drive *sm_current_drive;
    thread_local static unsigned int sm_speed;

    static void StaticCallback(long int i, paranoia_cb_mode_t cbm)
    {
	// Only print anything if something's going wrong
	if (cbm != PARANOIA_CB_READ && cbm != PARANOIA_CB_VERIFY
	    && cbm != PARANOIA_CB_OVERLAP)
	{
	    TRACE << "paranoia callback "
                  << sm_current_drive->ioctl_device_name << ": "
                  << i << " mode " << cbm << "\n";

	    if (cbm != PARANOIA_CB_CACHEERR && cbm != PARANOIA_CB_DRIFT)
	    {
		// For serious errors, try and slow down the drive

		if (sm_speed > 1) {
		    sm_speed /= 2;
                    TRACE << "Setting speed to " << sm_speed << "x\n";

                    cdrom_drive *d = sm_current_drive;
                    if (d)
                        cdda_speed_set(d, sm_speed);
                }
	    }
	}
    }

public:
    ParanoiaStream(cdrom_drive *cdt, int first_sector, int last_sector)
	: m_drive(cdt),
	  m_paranoia(paranoia_init(cdt)),
	  m_first_sector(first_sector),
	  m_last_sector(last_sector),
	  m_sector(first_sector-1),
	  m_data(NULL),
	  m_skip(2352)
    {
	sm_speed = 64;
	int rc = cdda_speed_set(cdt, 64);
        printf("speed(64): %d\n", rc);
        rc = cdda_speed_set(cdt, 48);
        printf("speed(48): %d\n", rc);

	TRACE << "Read starting\n";
	paranoia_modeset(m_paranoia, 
			 PARANOIA_MODE_FULL - PARANOIA_MODE_NEVERSKIP);
	paranoia_seek(m_paranoia, first_sector, SEEK_SET);
    }

    ~ParanoiaStream()
    {
	paranoia_free(m_paranoia);
    }

    // Being a SeekableStream
    unsigned GetStreamFlags() const { return READABLE|SEEKABLE; }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len,
		     size_t *pwrote);
    unsigned int Seek(uint64_t pos);
    uint64_t Tell();
    uint64_t GetLength();
    unsigned SetLength(uint64_t);
};

thread_local cdrom_drive *ParanoiaStream::sm_current_drive = NULL;
thread_local unsigned int ParanoiaStream::sm_speed = 64;

unsigned ParanoiaStream::ReadAt(void *buffer, uint64_t pos, size_t len,
				size_t *pread)
{
    /** @todo Isn't thread-safe like the other ReadAt's. Add a mutex. */

    if (pos != Tell())
	Seek(pos);

    if (m_sector == m_last_sector && !m_data)
    {
	// EOF
	*pread = 0;
	return 0;
    }

    if (!m_data)
    {
	++m_sector;
//	TRACE << "Reading sector " << m_sector << "\n";

	if (!sm_current_drive)
	    sm_current_drive = m_drive;
	m_data = (const char*)paranoia_read(m_paranoia, &StaticCallback);
	if (sm_current_drive == m_drive)
	    sm_current_drive = NULL;

	if (!m_data)
	{
	    TRACE << "Unrecoverable error!\n";
	    return EIO;
	}

//	TRACE << "Got sector " << m_sector << "\n";
	m_skip = 0;
    }

    unsigned int lump = 2352 - m_skip;
    if (lump > len)
	lump = (unsigned int)len;

    memcpy(buffer, m_data + m_skip, lump);

    m_skip += lump;
    if (m_skip == 2352)
    {
	m_data = NULL;
    }

//    TRACE << "Returned bytes " << pos << ".." << (pos+lump) << "\n";

    *pread = lump;
    return 0;
}

unsigned ParanoiaStream::WriteAt(const void*, uint64_t, size_t, size_t*)
{
    return EPERM; // We're read-only
}

unsigned ParanoiaStream::Seek(uint64_t pos)
{
    if (pos == Tell())
	return 0;

    uint64_t current = Tell();
    TRACE << "Tell()=" << current/2352 << ":" << current%2352
	  << " pos=" << pos/2352 << ":" << pos%2352 << ", seeking\n";

    m_sector = (int)(m_first_sector + pos/2352);
    m_skip = (unsigned int)(pos % 2352);

    assert(pos == Tell());

    TRACE << "Seeking to sector " << m_sector << "\n";

    paranoia_seek(m_paranoia, m_sector, SEEK_SET);

    m_data = NULL;
    return 0;
}

uint64_t ParanoiaStream::Tell()
{
    return (m_sector - m_first_sector) * UINT64_C(2352) + m_skip;
}

uint64_t ParanoiaStream::GetLength()
{
    return (m_last_sector - m_first_sector + 1) * UINT64_C(2352);
}

unsigned int ParanoiaStream::SetLength(uint64_t)
{
    return EPERM;
}

std::unique_ptr<util::Stream> LocalAudioCD::GetTrackStream(unsigned int track)
{
    return std::unique_ptr<util::Stream>(
	new ParanoiaStream((cdrom_drive*)m_cdt,
			   m_toc[track].first_sector,
			   m_toc[track].last_sector));
}

} // namespace import
