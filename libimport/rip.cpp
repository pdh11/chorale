#include "scsi.h"
#include "ripping_engine.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <map>
#include <vector>

namespace rip {

enum { RIP_NONE = 100, RIP_ALL = 101 };

static void feature(const char *s, int n)
{
    printf("%s %s\n", s, n ? "yes" : "no");
}

#if 0
enum {
    MSF = 0x01,
    C2  = 0x06,
    SUB = 0x18
};

static void msf(int lba, unsigned char *msf)
{
    *msf = (unsigned char)(lba/(75*60));
    msf[1] = (unsigned char)((lba/75) % 60);
    msf[2] = (unsigned char)(lba % 75);
}

/** Attempt ripping with the given flags
 */
bool try_rip(import::ScsiTransport *scsi, unsigned sector,
	     unsigned xflags, FILE *f)
{
    bool use_msf = (xflags & MSF);
    unsigned c2 = (xflags & C2) >> 1;
    if (c2 == 3)
	return false;
    unsigned subc = (xflags & SUB) >> 3;
    if (subc == 3)
	return false;

    unsigned char command[12];
    memset(command, '\0', sizeof(command));
    command[0] = use_msf ? import::READ_CD_MSF : import::READ_CD;
    command[1] = 0;
    if (use_msf)
    {
	msf(sector, command+3);
	msf(sector+1, command+6);
    }
    else
    {
	command[3] = (unsigned char)(sector>>16);
	command[4] = (unsigned char)(sector>>8);
	command[5] = (unsigned char)sector;
	command[8] = 1;
    }
    unsigned size = 2352;

    unsigned char flags = 0x10;
    const char *sc2 = "none";
    if (c2 == 1)
    {
	flags |= 0x02;
	size += 294;
	sc2 = "C2b ";
    }
    else if (c2 == 2)
    {
	flags |= 0x04;
	size += 296;
	sc2 = "C2bb";
    }
    command[9] = flags;

    const char *ssc = "none";

    if (subc == 1)
    {
	command[10] = 1; // RAW
	size += 96;
	ssc = "raw ";
    }
    else if (subc == 2)
    {
	command[10] = 2; // Q
	size += 16;
	ssc = "Q   ";
    }

    unsigned char buffer[4096];

    memset(buffer, 0xCD, sizeof(buffer));

    unsigned rc = scsi->SendCommand(command, 12, buffer, size);

    unsigned last = size;
    while (last > 0 && buffer[last-1] == 0xCD)
	--last;

    printf("%s %s %s: %s (%u/%u)\n", use_msf ? "MSF" : "LBA", sc2, ssc,
	   rc ? "ERR" : "OK", last, size);
    if (f && !rc)
    {
	fwrite(buffer, 2352, 1, f);
    }
    return rc == 0;
}
#endif

static unsigned char wav_header[] =
{
    'R', 'I', 'F', 'F',     // Identifier
    0x24, 0xA0, 0x98, 0x7B, // Size of whole file, minus 8, little-endian
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',     // Subchunk ID
    16, 0, 0, 0,            // Subchunk size, excl header
    1, 0,                   // AudioFormat, 1=PCM
    2, 0,                   // NumChannels, 2=stereo
    0x44, 0xAC, 0, 0,       // SampleRate, 44100=0xAC44
    0x10, 0xB1, 2, 0,       // ByteRate, 44100*4=0x2b110
    4, 0,                   // Bytes per sample tuple (2*2)
    16, 0,                  // Bits per sample
    'd', 'a', 't', 'a',     // Subchunk ID
    0x00, 0xA0, 0x98, 0x7B  // Subchunk size, ie size of whole file minus 44
};

static unsigned rip_track(import::RippingEngine *re, unsigned int n,
			  unsigned int beginlba, unsigned int endlba)
{
    char file[20];
    sprintf(file, "%02u.wav", n+1);
    FILE *f = fopen(file, "wb");
    if (!f)
	return errno;

    unsigned int file_size = (endlba - beginlba)*2352 + 44;
    write_le32(wav_header+4, file_size-8);
    write_le32(wav_header+40, file_size-44);

    size_t sz = fwrite(wav_header, 44, 1, f);
    if (!sz)
    {
	fclose(f);
	unlink(file);
	return errno;
    }

    unsigned char sector[2352];
    for (unsigned int i=beginlba; i<endlba; ++i)
    {
	unsigned rc = re->ReadSector(i, sector);
	if (rc)
	{
	    fclose(f);
	    unlink(file);
	    return rc;
	}
	sz = fwrite(sector, 2352, 1, f);
	if (!sz)
	{
	    fclose(f);
	    unlink(file);
	    return errno;
	}
    }
    fclose(f);
    return 0;
}

static unsigned rip(import::ScsiTransport *scsi, unsigned int track)
{
    unsigned char command[12];
    memset(command, '\0', sizeof(command));

    unsigned char buffer[4096];
    memset(buffer, '\0', sizeof(buffer));

    command[0] = import::MODE_SENSE_10;
    command[2] = import::CD_CAPABILITIES;
    command[8] = 18;

    unsigned rc = scsi->SendCommand(command, 10, buffer, 18);

    if (rc)
    {
	fprintf(stderr, "SendCommand returned %u: %s\n", rc, strerror(rc));
	return rc;
    }

    feature("read cd   ", buffer[10] & 1);
    feature("read mode2", buffer[10] & 2);
    feature("cdda      ", buffer[13] & 1);
    feature("accurate  ", buffer[13] & 2);
    feature("c2 ptrs   ", buffer[13] & 0x10);
    feature("lock      ", buffer[14] & 1);
    feature("discpres  ", buffer[15] & 4);
    printf( "buffer     %uKbytes\n",
	    buffer[12+8]*256 + buffer[13+8]);
    printf( "speed      %ux (%uKbytes/s)\n", 
	    (buffer[16]*256 + buffer[17]) / 176,
	    buffer[16]*256 + buffer[17]);

    memset(command, '\0', sizeof(command));
    command[0] = import::READ_TOC_PMA_ATIP;
    command[1] = 0x02;
    command[2] = 0x02;
    command[7] = (unsigned char)(sizeof(buffer) >> 8);
    command[8] = (unsigned char)sizeof(buffer);

    rc = scsi->SendCommand(command, 10, buffer, sizeof(buffer));
    if (rc)
    {
	if (errno == EIO)
	{
	    printf("Can't read TOC -- no disc present?\n");
	    return 0;
	}
	perror("readtoc");
	return errno;
    }

    unsigned toclen = (buffer[0] << 8) + buffer[1];
    printf("%u bytes of toc\n", toclen);

    if (!toclen)
	return ENODEV;

    typedef std::map<unsigned, const unsigned char*> points_t;
    points_t points;

    unsigned int entries = (toclen-2) / 11;

    const unsigned char realbegin[] = { 1,0x10,0,0, 0,0,0,0, 0,2,0,0 };

    unsigned last_lba = 150;

    for (unsigned int entry=0; entry<entries; ++entry)
    {
	const unsigned char *ptr = buffer + 4 + entry*11;
	{
	    printf("[%02x] %3u %2u:%02u:%02u %02x\n", ptr[0], ptr[3],
		   ptr[8], ptr[9], ptr[10],
		   ptr[1]);
	    unsigned lba = ptr[8]*60*75 + ptr[9]*75 + ptr[10];

	    if (ptr[3] != 0 && ptr[0] == 1)
		points[ptr[3]] = ptr;

	    if (ptr[3] == 162 && ptr[0] == 1)
		last_lba = lba;

	    if (ptr[3] == 1 && ptr[0] == 1 && lba > 150)
	    {
		printf("Possible hidden track?\n");
		points[0] = realbegin;
	    }
	}
    }

    unsigned int t = 1;

    std::vector<unsigned> begins;
    std::vector<unsigned> ends;

    for (points_t::const_iterator i = points.begin();
	 i != points.end();
	 ++i)
    {
	if (i->first < 99)
	{
	    points_t::const_iterator j = i;
	    ++j;
	    if (j != points.end())
	    {
		if (j->first > 99)
		    j = points.find(162);
	    }
	    if (j != points.end())
	    {
		const unsigned char *p1 = i->second;
		const unsigned char *p2 = j->second;
		if (p1[1] == 0x10)
		{
		    unsigned lba1 = p1[8]*75*60 + p1[9]*75 + p1[10];
		    unsigned lba2 = p2[8]*75*60 + p2[9]*75 + p2[10];
		    printf(" %2u %02u:%02u:%02u..%02u:%02u:%02u (%u..%u)\n",
			   t, p1[8], p1[9], p1[10],  p2[8], p2[9], p2[10],
			   lba1, lba2);
		    begins.push_back(lba1);
		    ends.push_back(lba2);
		    ++t;
		}
	    }
	}
    }

    /* Carter USM "30 Something"
     *   -- has points 1-9, 16, 17 for its 11 tracks
     *   -- starts track 6 at "25:35:115" (which presumably means 25:36:40)
     *
     * 2 Many DJs "As Heard On Radio Soulwax Part 2"
     *   -- has "hidden" track before track 1 (msf 00:02:00 onwards)
     *   -- Plextor will rip it, NEC DVD_RW errors out
     */
    
    if (track == RIP_NONE)
    {
	/* NEC CD-ROM DRIVE:466
	 *   -- accepts READ_CD but not READ_CD_MSF
	 *   -- doesn't advertise C2 support but does return bits and/or block
	 *   -- returns Q and RAW
	 * HP CD-Writer+ 9200
	 *   -- accepts both READ_CD and READ_CD_MSF
	 *   -- can't return bits or block (but doesn't advertise C2 support)
	 *   -- returns Q and RAW
	 * SONY CD-RW CRX820E (slimline)
	 *   -- can do everything
	 * TEAC CD-W524E
	 *   -- can do everything
	 * NEC DVD_RW ND-4570A
	 *   -- can do everything
	 * AOPEN 16DVD-ROM/AMH
	 *   -- can't read RAW subchannel (reads Q)
	 *   -- fails if you read neither C2 nor subchannel (?!)
	 * PLEXTOR CD-R PX-W4824A
	 *   -- can't read C2 bits-and-block
	 *   -- can do everything else including Q and RAW
	 *
	 * Suggests failover order:
	 *   LBA + C2b + Q  (Plex,AOP,TEAC,Sony,NEC*2)
	 *   LBA       + Q  (HP)
	 *   LBA
	 *   
	 */
	unsigned caps = 0;
	for (unsigned int i=0; i<0x20; ++i)
	{
	    if ( (i & import::USE_C2B) && (i & import::USE_C2BB))
		continue;
	    if ( (i & import::USE_Q) && (i & import::USE_RAW))
		continue;

	    unsigned size = import::GetSectorSize(i);

	    memset(buffer, 0xCD, size);

	    import::ReadCD rcd(i, 150, 1);
	    rc = rcd.Submit(scsi, buffer, size);

	    unsigned last = size;
	    if (!rc)
	    {
		while (last > 0 && buffer[last-1] == 0xCD)
		    --last;
	    }
	    
	    printf("%u: %u %u/%u\n", i, rc, last, size);
	    if (!rc && last==size)
		caps |= (1<<i);
	}
	printf("   -----------LBA------------ -----------MSF------------\n");
	printf("   --none-- --C2B--- --C2BB-- --none-- --C2B--- --C2BB--\n");
	printf("   0  Q RAW 0  Q RAW 0  Q RAW 0  Q RAW 0  Q RAW 0  Q RAW\n");
	printf("  ");
	for (unsigned msf=0; msf<2; ++msf)
	    for (unsigned c2=0; c2<6; c2+=2)
		for (unsigned sc=0; sc<24; sc+=8)
		    printf(" %s ", caps & (1<<(msf+c2+sc)) ? "Y" : "N");
	printf("\n");
    }
    else
    {
	import::RippingEngine re(scsi, import::USE_Q|import::USE_C2B, 150, 
				 last_lba);

	if (track == RIP_ALL)
	{
	    rc = 0;
	    for (unsigned int i=0; i<begins.size(); ++i)
	    {
		rc = rip_track(&re, i, begins[i], ends[i]);
		if (rc)
		    break;
	    }
	}
	else if (track <= begins.size())
	{
	    rc = rip_track(&re, track-1, begins[track-1], ends[track-1]);
	}
    }

    return rc;
}

static unsigned rip(const char *device, int track)
{
    std::auto_ptr<import::ScsiTransport> st;
    unsigned rc = import::CreateScsiTransport(device, &st);
    if (rc)
	return rc;

    return rip(st.get(), track);
}

static void Usage(FILE *f)
{
    fprintf(f,
	    "Usage: rip [options] <device>\n"
	    "Test and rip an audio CD.\n"
	    "\n"
	    "The options are:\n"
	    " -a, --all     Rip all tracks\n"
	    " -t, --track N Rip only track N (1 to 99)\n"
	    " -h, --help    This help\n"
	    "\n"
	    "If neither -a nor -t is given, the first few sectors are ripped as a test.\n"
	    "\n"
	    "From " PACKAGE_STRING " (" PACKAGE_WEBSITE ") built on " __DATE__ ".\n");
}

static int Main(int argc, char *argv[])
{
    static const struct option options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "all", no_argument, NULL, 'a' },
	{ "track", required_argument, NULL, 't' },
	{ NULL, 0, NULL, 0 }
    };

    unsigned track = RIP_NONE;

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "hat:", options, &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	    
	case 'a':
	    track = RIP_ALL;
	    break;

	case 't':
	    track = (int)strtoul(optarg, NULL, 10);
	    if (track>99)
	    {
		Usage(stderr);
		return 1;
	    }
	    break;

	default:
	    Usage(stderr);
	    return 1;
	}
    }

    if (argc-optind != 1)
    {
	Usage(stderr);
	return 1;
    }

    unsigned rc = rip(argv[optind], track);
    return rc ? 1 : 0;
}

} // namespace rip

int main(int argc, char *argv[])
{
    return rip::Main(argc, argv);
}
