#include "libutil/file_stream.h"
#include "libutil/file.h"
#include "libutil/partial_stream.h"
#include "libutil/endian.h"
#include "config.h"
#include <string.h>
#include <getopt.h>
#include <stdio.h>

namespace doctor {

namespace {

static bool s_all = false;
static bool s_frames = false;

static void Usage(FILE *f)
{
    fprintf(f,
"Usage: doctor [options] <file>...\n"
"Report potential problems with audio files.\n"
"\n"
"Options:\n"
" -a, --all    Print all information (not just problems)\n"
" -f, --frames Report every single frame in each file\n"
" -h, --help   These hastily-scratched notes\n"
"\n"
"From " PACKAGE_STRING " (" PACKAGE_WEBSITE ") built on " __DATE__ ".\n"
	);
}

static uint32_t read_unsync32(const unsigned char *src)
{
    return (src[0] << 21)
	| (src[1] << 14)
	| (src[2] << 7)
	| (src[3]);
}

static unsigned CheckID3v2(util::Stream *st, uint64_t *pos, bool *ok)
{
    *pos = 0;

    char header[10];
    unsigned rc = st->ReadAll(&header, 10);
    if (rc)
    {
	perror("read");
	return rc;
    }

    if (!memcmp(header, "ID3", 3))
    {
	unsigned char version = header[3];
	if (version < 2 || version > 4)
	{
	    printf("  Bad ID3v2\n");
	    *ok = false;
	}
	else
	{
	    unsigned len = (header[6] << 21)
		| (header[7] << 14)
		| (header[8] << 7)
		| (header[9]);
	    
	    uint64_t sz = st->GetLength();
    
	    if (len > sz-10)
	    {
		printf("  Bad ID3v2\n");
		*ok = false;
	    }
	    if (s_all)
	    {
		printf("  %u bytes of ID3v2.%u\n", len, version);
	    }
	    //unsigned flags = header[4];

	    uint64_t frame = 0;
	    while (frame < len)
	    {
		unsigned char frameheader[10];
		st->Seek(10 + frame);
		rc = st->ReadAll(&frameheader, version == 2 ? 6 : 10);
		if (frameheader[0] == 0)
		{
		    if (s_all)
			printf("    %u bytes of padding\n",
                               unsigned(len-frame));
		    break;
		}

		unsigned framelen = 0;
		if (version == 4)
		{
		    framelen = read_unsync32(frameheader+4);
		}
		else if (version == 3)
		{
		    framelen = read_be32(frameheader+4);
		} 
		else if (version == 2)
		{
		    framelen = read_be24(frameheader+3);
		}

		if (framelen)
		{
		    std::string tag(frameheader, 
				    frameheader + ((version==2) ? 3 : 4));
		    printf("    %u-byte frame (%s)\n", framelen, tag.c_str());
		}
		else
		{
		    *ok = false;
		    printf("    Bad frame length %u\n", framelen);
		}
		
		frame += framelen + ((version==2) ? 6 : 10);
	    }

	    *pos = len+10;
	}
    }

    return 0;
}

static unsigned CheckSync(util::Stream *st, uint64_t *skip, bool *ok)
{
    st->Seek(*skip);

    uint64_t pos = *skip;
    unsigned char prev = 0;
    uint64_t junk = 0;
    do {
	unsigned char ch;
	unsigned rc = st->ReadAll(&ch, 1);
	if (rc)
	{
	    perror("read");
	    return rc;
	}
	if (prev == 0xFF && (ch & 0xE0) == 0xE0)
	{
	    // Found sync
	    break;
	}
	prev = ch;
	++junk;
	++pos;
    } while (1);

    --junk;
    --pos;
	
    if (junk)
    {
	printf("  %u bytes of prejunk\n", (unsigned)junk);
	*ok = false;
    }

    *skip = pos;

    return 0;
}

union FrameHeader
{
    uint32_t x;
    struct {
	unsigned emphasis : 2;
	unsigned original : 1;
	unsigned copyright : 1;
	unsigned extension : 2;
	unsigned channelmode : 2;
	unsigned privatebit : 1;
	unsigned padding : 1;
	unsigned samplerate : 2;
	unsigned bitrate : 4;
	unsigned protection: 1;
	unsigned layer : 2;
	unsigned version : 2;
	unsigned sync : 11;
    } s;
};

static unsigned BitRate(FrameHeader fh)
{
    if (fh.s.bitrate == 0xF)
	return 0;

    static const uint16_t bitrate_table[][16] = {
	{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
	{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
	{ 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 },
	{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
	{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }
    };

    int version = 0;
    if (fh.s.version == 0) // v2.5
	version = 2;
    else if (fh.s.version == 2) // v2
	version = 2;
    else if (fh.s.version == 3) // v1
	version = 1;

    int layer = 4 - fh.s.layer;

    if (version == 1 && layer == 1)
        return 1000 * bitrate_table[0][fh.s.bitrate];
    else if (version == 1 && layer == 2)
	return 1000 * bitrate_table[1][fh.s.bitrate];
    else if (version == 1 && layer == 3)
	return 1000 * bitrate_table[2][fh.s.bitrate];
    else if (version == 2 && layer == 1)
	return 1000 * bitrate_table[3][fh.s.bitrate];
    else if (version == 2 && (layer == 2 || layer == 3))
	return 1000 * bitrate_table[4][fh.s.bitrate];

    return 0;
}

static unsigned SampleRate(FrameHeader fh)
{
    static const uint16_t sample_rate_table[][4] = {
	{ 44100, 48000, 32000, 0 },
	{ 22050, 24000, 16000, 0 },
	{ 11025, 12000,  8000, 0 }
    };

    if (fh.s.version == 3)
	return sample_rate_table[0][fh.s.samplerate];
    else if (fh.s.version == 2)
	return sample_rate_table[1][fh.s.samplerate];
    else if (fh.s.version == 0)
	return sample_rate_table[2][fh.s.samplerate];

    return 0;
}

static unsigned FrameLength(FrameHeader fh)
{
    unsigned bitrate = BitRate(fh);
    if (!bitrate)
	return 0;
    unsigned samplerate = SampleRate(fh);
    if (!samplerate)
	return 0;
    if (fh.s.layer == 3)
	return (((12*bitrate) / samplerate) + fh.s.padding) * 4;

    unsigned samples = 1152;
    if ((fh.s.version == 0 || fh.s.version == 2) && fh.s.layer == 1)
	samples = 576;

    return (samples*bitrate)/(8*samplerate) + fh.s.padding;
}

static unsigned WalkFrames(util::Stream *st, uint64_t pos, bool *ok)
{
    uint64_t end = st->GetLength();
    unsigned int nframes = 0;
    unsigned int nsilent = 0;
    bool started = false;
    unsigned int bitrate = 0;
    bool vbr = false;
    uint64_t streamlen = end-pos;

    while (pos < (end-4))
    {
	unsigned char header[4];
	st->Seek(pos);
	unsigned rc = st->ReadAll(header, 4);
	if (rc)
	    return rc;

	FrameHeader fh;
	fh.x = read_be32(header);
	if (fh.s.sync != 0x7FF)
	{
	    printf("  Sync lost at %llu (%08x %08x)\n", (long long unsigned)pos,
		   fh.x, fh.s.sync);
	    *ok = false;
	    return 0;
	}
	
	unsigned int len = FrameLength(fh);
	if (!len)
	{
	    printf("  Bogus header %08x at %llu\n", fh.x,
		   (long long unsigned)pos);
	    *ok = false;
	    return 0;
	}

	if (s_frames)
	{
	    printf("  0x%08llx: frame %u\n", (unsigned long long)pos, nframes);
	}

	unsigned char frame[8192];

	rc = st->ReadAll(frame, len-4);
	if (rc)
	    return rc;

	unsigned int zeroes;
	if (fh.s.version == 3)
	{
	    if (fh.s.channelmode == 3)
		zeroes = 17;
	    else
		zeroes = 32;
	} 
	else
	{
	    if (fh.s.channelmode == 3)
		zeroes = 9;
	    else
		zeroes = 17;
	}

	bool silent = true;
	for (unsigned int i=0; i<zeroes && silent; ++i)
	{
	    if (frame[i])
		silent = false;
	}

	if (silent && !started)
	{
	    ++nsilent;
	    if (!memcmp(frame + zeroes, "Xing", 4)
		|| !memcmp(frame + zeroes, "Info", 4))
	    {
		if (s_all)
		{
		    printf("  Info header in frame %u\n", nframes);
		    printf("    Frame count %u\n", read_be32(frame+zeroes+8));
		    printf("    Stream length %u\n", read_be32(frame+zeroes+12));
		}
	    }
	    else if (!memcmp(frame+32, "VBRI", 4))
	    {
		if (s_all)
		    printf("  VBRI header in frame %u\n", nframes);
	    }
	    else
	    {
		started = true;
		if (s_all)
		    printf("  Audio begins in frame %u\n", nframes);
	    }
	}
	else
	{
	    if (!started)
	    {
		started = true;
		if (s_all)
		    printf("  Audio begins in frame %u\n", nframes);
	    }
	    nsilent = 0;
	}

	if (started)
	{
	    unsigned frame_bitrate = BitRate(fh);
	    if (!bitrate)
		bitrate = frame_bitrate;
	    else if (bitrate != frame_bitrate)
		vbr = true;
	}

	++nframes;

	pos += len;
    }

    if (s_all)
    {
	if (nsilent)
	    printf("  Final %u frames silent\n", nsilent);
	printf("  %u frames\n", nframes);
	if (vbr)
	    printf("  Variable bitrate\n");
	else
	    printf("  Bitrate=%u CBR\n", bitrate);
	printf("  MP3 stream length %llu\n", (unsigned long long)streamlen);
    }

    return 0;
}

static unsigned Doctor(const char *filename)
{
    if (util::GetExtension(filename) != "mp3")
    {
	fprintf(stderr, "Ignoring non-MP3 '%s'\n", filename);
	return 0;
    }

    printf("%s:\n", filename);

    std::unique_ptr<util::Stream> st;

    unsigned rc = util::OpenFileStream(filename, util::READ|util::SEQUENTIAL,
				       &st);
    if (rc)
    {
	perror("open");
	return rc;
    }

    //uint64_t sz = st->GetLength();
    uint64_t pos = 0;
    bool ok = true;

    /** Look for ID3v2 */

    rc = CheckID3v2(st.get(), &pos, &ok);
    if (rc)
	return rc;

    /** Look for sync */

    rc = CheckSync(st.get(), &pos, &ok);

    /** Walk frames */

    rc = WalkFrames(st.get(), pos, &ok);

    if (ok)
	printf("  OK\n");

    return 0;
}

int Main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "help", no_argument, NULL, 'h' },
	{ "all", no_argument, NULL, 'a' },
	{ "frames", no_argument, NULL, 'f' },
    };

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "hfa", options, &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	case 'a':
	    s_all = true;
	    break;
	case 'f':
	    s_frames = true;
	    break;
	default:
	    Usage(stderr);
	    return 1;
	}
    }

    if (argc == optind)
    {
	Usage(stderr);
	return 1;
    }

    bool ok = true;
    for (int i = optind; i<argc; ++i)
    {
	unsigned rc = Doctor(argv[i]);
	ok &= (rc == 0);
    }

    return ok ? 0 : 1;
}

} // anon namespace

} // namespace doctor

int main(int argc, char *argv[])
{
    return doctor::Main(argc, argv);
}
