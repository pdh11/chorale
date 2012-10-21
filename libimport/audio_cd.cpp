#include "config.h"
#include "audio_cd.h"

#ifdef HAVE_LIBCDIOP

#include "libutil/trace.h"
#include <cdio/cdio.h>
#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include <cdio/paranoia.h>
#include <errno.h>
#include <string.h>

namespace import {

unsigned LocalAudioCD::Create(const std::string& device, AudioCDPtr *pcd)
{
    *pcd = AudioCDPtr(NULL);
    
    cdrom_drive_t *cdt =  cdio_cddap_identify(device.c_str(),
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
	return (unsigned)errno;
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

    for (track_t i=1; i<=total; ++i) // Peculiar 1-based numbering
    {
	if (cdio_cddap_track_audiop(cdt, i))
	{
	    TocEntry te;
	    te.first_sector = cdio_cddap_track_firstsector(cdt, i);
	    te.last_sector  = cdio_cddap_track_lastsector(cdt, i);
//	    TRACE << "Track " << i << " " << te.firstsector
//		  << ".." << te.lastsector << "\n";
	    
	    toc.push_back(te);

	    total_sectors += (unsigned)(te.last_sector - te.first_sector + 1);
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
	cdio_cddap_close((cdrom_drive_t*)m_cdt);
//    TRACE << "~LocalAudioCD\n";
}

class ParanoiaStream: public util::SeekableStream
{
    cdrom_paranoia_t *m_paranoia;
    int m_first_sector;
    int m_last_sector;
    int m_sector;
    const char *m_data;
    unsigned int m_skip;

public:
    ParanoiaStream(cdrom_drive_t *cdt, int first_sector, int last_sector)
	: m_paranoia(paranoia_init(cdt)),
	  m_first_sector(first_sector),
	  m_last_sector(last_sector),
	  m_sector(first_sector),
	  m_data(NULL),
	  m_skip(0)
    {
	paranoia_modeset(m_paranoia,
			 PARANOIA_MODE_FULL - PARANOIA_MODE_NEVERSKIP);
	paranoia_seek(m_paranoia, first_sector, SEEK_SET);
    }

    ~ParanoiaStream()
    {
	paranoia_free(m_paranoia);
    }

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, size_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, size_t pos, size_t len,
		     size_t *pwrote);
    void Seek(pos64 pos);
    pos64 Tell();
    pos64 GetLength();
    unsigned SetLength(pos64);
};

unsigned ParanoiaStream::ReadAt(void *buffer, size_t pos, size_t len,
				size_t *pread)
{
    /** @todo Isn't thread-safe like the other ReadAt's. Add a mutex. */

    if (pos != Tell())
	Seek(pos);

    if (m_sector == m_last_sector+1 && !m_data)
    {
	// EOF
	*pread = 0;
	return 0;
    }

    if (!m_data)
    {
	m_data = (const char*)paranoia_read(m_paranoia, NULL);
	m_skip = 0;
	++m_sector;
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

    *pread = lump;
    return 0;
}

unsigned ParanoiaStream::WriteAt(const void*, size_t, size_t, size_t*)
{
    return EPERM; // We're read-only
}

void ParanoiaStream::Seek(pos64 pos)
{
    /* NB this is only accurate to the nearest sector */
    m_sector = (int)(m_first_sector + pos/2352);

    paranoia_seek(m_paranoia, m_sector, SEEK_SET);

    m_skip = (unsigned int)(pos % 2352);
    m_data = NULL;
}

util::SeekableStream::pos64 ParanoiaStream::Tell()
{
    return (m_sector - m_first_sector) * 2352 + m_skip;
}

util::SeekableStream::pos64 ParanoiaStream::GetLength()
{
    return (m_last_sector - m_first_sector + 1) * 2352;
}

unsigned int ParanoiaStream::SetLength(pos64)
{
    return EPERM;
}

util::SeekableStreamPtr LocalAudioCD::GetTrackStream(unsigned int track)
{
    return util::SeekableStreamPtr(new ParanoiaStream((cdrom_drive_t*)m_cdt,
						      m_toc[track].first_sector,
						      m_toc[track].last_sector));
}

} // namespace import

#endif // HAVE_LIBCDIOP
