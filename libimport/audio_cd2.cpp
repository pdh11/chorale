#if 0
#include "audio_cd.h"
#include "config.h"
#include "libutil/stream.h"
#include "libutil/trace.h"
#include "libutil/counted_pointer.h"
#include <errno.h>
#include <string.h>
#include <map>

#if HAVE_LINUX_CDROM_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

namespace import {

static void msf(int lba, unsigned char *msf)
{
    *msf = (unsigned char)(lba/(75*60));
    msf[1] = (unsigned char)((lba/75) % 60);
    msf[2] = (unsigned char)(lba % 75);
}

static unsigned readcommand(int fd, const unsigned char command[12],
			    void *buffer, size_t len)
{
    for (unsigned int tries = 0; tries < 4; ++tries)
    {
	cdrom_generic_command cgc;
	memset(&cgc, '\0', sizeof(cgc));
	request_sense rs;
	memset(&rs, '\0', sizeof(rs));

	memcpy(cgc.cmd, command, 12);
	cgc.buffer = (unsigned char*)buffer;
	cgc.buflen = (unsigned)len;
	cgc.sense = &rs;
	cgc.data_direction = CGC_DATA_READ;

	int rc = ::ioctl(fd, CDROM_SEND_PACKET, &cgc);
	if (rc<0)
	{
	    TRACE << "ioctl failed (" << errno << ")\n";
	    continue;
	}

//	if (cgc.buflen)
//	    printf("%u bytes of buffer left\n", (unsigned)cgc.buflen);
    
	if (!rs.sense_key)
	{
//	    printf("ioctl ok\n");
	    return 0;
	}

//	printf("sense key is %u\n", rs.sense_key);
    }

    return EIO;
}

static unsigned atomcommand(int fd, const unsigned char command[12])
{
    for (unsigned int tries = 0; tries < 4; ++tries)
    {
	cdrom_generic_command cgc;
	memset(&cgc, '\0', sizeof(cgc));
	request_sense rs;
	memset(&rs, '\0', sizeof(rs));

	memcpy(cgc.cmd, command, 12);
	cgc.sense = &rs;
	cgc.data_direction = CGC_DATA_NONE;

	int rc = ::ioctl(fd, CDROM_SEND_PACKET, &cgc);
	if (rc<0)
	{
	    TRACE << "ioctl failed (" << errno << ")\n";
	    continue;
	}

//	if (cgc.buflen)
//	    printf("%u bytes of buffer left\n", (unsigned)cgc.buflen);
    
	if (!rs.sense_key)
	{
//	    printf("ioctl ok\n");
	    return 0;
	}

//	printf("sense key is %u\n", rs.sense_key);
    }

    return EIO;
}

unsigned LocalAudioCD::Create(const std::string& device, AudioCDPtr *pcd)
{
    *pcd = AudioCDPtr(NULL);

    int fd = ::open(device.c_str(), O_RDONLY|O_NONBLOCK);
    if (fd<0)
    {
	TRACE << "Can't open device (" << errno << ")\n";
	return errno;
    }


    unsigned char command[12];
    memset(command, '\0', sizeof(command));
    unsigned char buffer[4096];
    memset(buffer, '\0', sizeof(buffer));

    command[0] = GPCMD_MODE_SENSE_10;
    command[2] = GPMODE_CAPABILITIES_PAGE;
    command[8] = 10;

    unsigned rc = readcommand(fd, command, buffer, 10);
    if (rc)
    {
	TRACE << "readcommand failed\n";
	close(fd);
	return errno;
    }

//    for (unsigned int i=0; i<10; ++i)
//	printf("  %02X", buffer[i]);
//    printf("\n");

    unsigned caplen = (buffer[0] << 8) + buffer[1];
//    printf("%u bytes of capabilites\n", caplen);
    if (caplen > 100)
	caplen = 100;

    memset(command, '\0', sizeof(command));
    command[0] = GPCMD_MODE_SENSE_10;
    command[2] = GPMODE_CAPABILITIES_PAGE;
    command[7] = (unsigned char)(caplen >> 8);
    command[8] = (unsigned char)caplen;

    memset(buffer, '\0', 40);

    rc = readcommand(fd, command, buffer, caplen);
    if (rc)
    {
	TRACE << "readcommand2 failed\n";
	close(fd);
	return errno;
    }

/*
    for (unsigned int i=0; i<36; ++i)
	printf("  %02X", buffer[i]);
    printf("\n");

    feature("read cd   ", buffer[10] & 1);
    feature("read mode2", buffer[10] & 2);
    feature("cdda      ", buffer[13] & 1);
    feature("accurate  ", buffer[13] & 2);
    feature("c2 ptrs   ", buffer[13] & 0x10);
    feature("lock      ", buffer[14] & 1);
    feature("discpres  ", buffer[15] & 4);
    printf( "speed      %ux (%u)\n", 
	    (buffer[16]*256 + buffer[17]) / 176,
	    buffer[16]*256 + buffer[17]);
*/

    memset(command, '\0', sizeof(command));
    command[0] = GPCMD_SET_SPEED; // MMC-3 section 5.1.15
    command[2] = 0xFF;
    command[3] = 0xFF;

    rc = atomcommand(fd, command);
    if (rc)
    {
	TRACE << "atomcommand failed\n";
	// But carry on
    }

    memset(command, '\0', sizeof(command));
    command[0] = GPCMD_READ_TOC_PMA_ATIP;
    command[1] = 0x02;
    command[2] = 0x02;
    command[8] = 4;

    rc = readcommand(fd, command, buffer, 4);
    if (rc)
    {
	TRACE << "readcommand3 failed\n";
	close(fd);
	return errno;
    }

    unsigned toclen = (buffer[0] << 8) + buffer[1];
//    printf("%u bytes of toc\n", toclen);
    command[7] = toclen >> 8;
    command[8] = toclen;

    rc = readcommand(fd, command, buffer, toclen);
    if (rc)
    {
	TRACE << "readcommand4 failed\n";
	close(fd);
	return errno;
    }

    std::map<unsigned, unsigned> points;

    unsigned int entries = (toclen-2) / 11;

    for (unsigned int entry=0; entry<entries; ++entry)
    {
	const unsigned char *ptr = buffer + 4 + entry*11;
	if (ptr[0] == 1)
	{
	    //printf(" %3u %2u:%02u:%02u %02x\n", ptr[3],
//		   ptr[8], ptr[9], ptr[10],
//		   ptr[1]);
	    points[ptr[3]] = (ptr[8]*60 + ptr[9])*75 + ptr[10];
	}
    }

    toc_t toc;

    for (unsigned int i=0; i<99; ++i)
    {
	TocEntry te;
	if (points.find(i+1) == points.end())
	    break;
	te.first_sector = points[i+1];
	if (points.find(i+2) != points.end())
	    te.last_sector = points[i+2] - 1;
	else
	    te.last_sector = points[162];

	toc.push_back(te);
	TRACE << "Track " << i << " " << te.first_sector
	      << ".." << te.last_sector << "\n";
    }

    if (toc.empty())
    {
	close(fd);
	TRACE << "No audio tracks\n";
	return ENOENT;
    }

    TRACE << "Opened with " << toc.size() << " tracks\n";

    LocalAudioCD *cd = new LocalAudioCD();
    cd->m_toc = toc;
    cd->m_fd = fd;
    cd->m_total_sectors = points[162];
    *pcd = AudioCDPtr(cd);
    return 0;
}

LocalAudioCD::~LocalAudioCD()
{
    if (m_fd != -1)
	close(m_fd);
    if (m_sectors)
	delete[] m_sectors;
}

unsigned LocalAudioCD::ReadSector(unsigned int n, const uint8_t **pdata)
{
    /* We don't quite have free rein here -- the interface limits us
     * to 128Kbytes (47 sectors)
     */
    enum { SECTORS = 40 };

    if (!m_sectors)
    {
	m_sectors = new FullSector[SECTORS];
	m_first_sector = (unsigned)-1;
    }
    
//    TRACE << "n=" << n << " mfs=" << m_first_sector << "\n";

    if (n >= m_first_sector && n < (m_first_sector+SECTORS))
    {
	*pdata = m_sectors[n - m_first_sector].pcm;
	return 0;
    }

    unsigned endsector = n + SECTORS;
    if (endsector > m_total_sectors)
	endsector = m_total_sectors;

    unsigned char command[12];
    memset(command, '\0', sizeof(command));
    command[0] = GPCMD_READ_CD_MSF;
    command[1] = 0x0;
    msf(n, command+3);
    msf(endsector, command+6);
    command[9] = 0xFC; // sync, header=all, userdata edc, c2=bitsandblock
    command[10] = 1; // raw subchannel
    command[11] = 0;
	    
    unsigned rc = readcommand(m_fd, command, m_sectors,
			      (endsector-n)*sizeof(FullSector));
    if (rc)
    {
	TRACE << "readcommandR failed (" << rc << ")\n";
	return rc;
    }
    
    m_first_sector = n;
    *pdata = m_sectors[0].pcm;
    return 0;
}

class RipStream: public util::SeekableStream
{
    LocalAudioCD *m_cd;
    int m_first_sector;
    int m_last_sector;
    int m_sector;
    const uint8_t *m_data;
    unsigned int m_skip;

public:
    RipStream(LocalAudioCD *cd, int first_sector, int last_sector)
	: m_cd(cd),
	  m_first_sector(first_sector),
	  m_last_sector(last_sector),
	  m_sector(first_sector-1),
	  m_data(NULL),
	  m_skip(2352)
    {
    }

    ~RipStream()
    {
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

unsigned RipStream::ReadAt(void *buffer, uint64_t pos, size_t len,
			   size_t *pread)
{
    /** @todo Isn't thread-safe like the other ReadAt's. Add a mutex. */

    if (pos != Tell())
	Seek(pos);

    size_t totalread = 0;

    do {
	if (!m_data)
	{
	    ++m_sector;

	    if (m_sector == m_last_sector)
	    {
		// EOF
		*pread = totalread;
		return 0;
	    }

	    unsigned rc = m_cd->ReadSector(m_sector, &m_data);
	    if (rc)
	    {
		TRACE << "readSector failed\n";
		return rc;
	    }
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

	totalread += lump;
	buffer = (char*)buffer + lump;
	len -= lump;
    } while (len);

    *pread = totalread;
    return 0;
}

unsigned RipStream::WriteAt(const void*, uint64_t, size_t, size_t*)
{
    return EPERM; // We're read-only
}

unsigned RipStream::Seek(uint64_t pos)
{
    if (pos == Tell())
	return 0;

    uint64_t current = Tell();
    TRACE << "Tell()=" << current/2352 << ":" << current%2352
	  << " pos=" << pos/2352 << ":" << pos%2352 << ", seeking\n";

    m_sector = (int)(m_first_sector + pos/2352);
    m_skip = (unsigned int)(pos % 2352);

    assert(pos == Tell());

    m_data = NULL;
    return 0;
}

uint64_t RipStream::Tell()
{
    return (m_sector - m_first_sector) * 2352 + m_skip;
}

uint64_t RipStream::GetLength()
{
    return (m_last_sector - m_first_sector + 1) * 2352;
}

unsigned int RipStream::SetLength(uint64_t)
{
    return EPERM;
}

std::auto_ptr<util::Stream> LocalAudioCD::GetTrackStream(unsigned int track)
{
    return std::auto_ptr<util::Stream>(
	new RipStream(this,
		      m_toc[track].first_sector,
		      m_toc[track].last_sector));
}

} // namespace import

#endif // HAVE_LINUX_CDROM_H
#endif
