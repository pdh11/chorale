#include "config.h"
#include "mp3_stream.h"
#include "libutil/trace.h"
#include <errno.h>
#include <string.h>

#if HAVE_MPG123

#include <endian.h>
#include <mpg123.h>

namespace tv {

#if 1

/** WAV header for 3hr PCM WAV
 *
 * You can't actually have an infinite WAV, and besides Receiver
 * clients lock up if sent a WAV over 2^31 bytes. We use 3 hours here
 * because it's the nearest round number (stopping after 3:00:00 looks
 * less like a bug to the user than stopping after 3:06:24).
 *
 * The PCM length is 48000*4*3600*3 bytes, or 2,073,600,000, or 0x7B98A000;
 * the file length is this plus the 44-byte header, or 0x7B98A02C.
 *
 * http://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 */
static const unsigned char three_hour_wav_header[] =
{
    'R', 'I', 'F', 'F',     // Identifier
    0x24, 0xA0, 0x98, 0x7B, // Size of whole file, minus 8, little-endian
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',     // Subchunk ID
    16, 0, 0, 0,            // Subchunk size, excl header
    1, 0,                   // AudioFormat, 1=PCM
    2, 0,                   // NumChannels, 2=stereo
    0x80, 0xBB, 0, 0,       // SampleRate, 48000=0xBB80
    0x00, 0xEE, 2, 0,       // ByteRate, 48000*4=0x2EE00
    4, 0,                   // Bytes per sample tuple (2*2)
    16, 0,                  // Bits per sample
    'd', 'a', 't', 'a',     // Subchunk ID
    0x00, 0xA0, 0x98, 0x7B  // Subchunk size, ie size of whole file minus 44
};

#else

/* One-minute WAV for testing
 *
 * 48000*4*60 = 11,520,000 or 0xAFC800
 */
static const unsigned char three_hour_wav_header[] =
{
    'R', 'I', 'F', 'F',     // Identifier
    0x24, 0xC8, 0xAF, 0x00, // Size of whole file, minus 8, little-endian
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',     // Subchunk ID
    16, 0, 0, 0,            // Subchunk size, excl header
    1, 0,                   // AudioFormat, 1=PCM
    2, 0,                   // NumChannels, 2=stereo
    0x80, 0xBB, 0, 0,       // SampleRate, 48000=0xBB80
    0x00, 0xEE, 2, 0,       // ByteRate, 48000*4=0x2EE00
    4, 0,                   // Bytes per sample tuple (2*2)
    16, 0,                  // Bits per sample
    'd', 'a', 't', 'a',     // Subchunk ID
    0x00, 0xC8, 0xAF, 0x00  // Subchunk size, ie size of whole file minus 44
};

#endif

MP3Stream::MP3Stream(std::unique_ptr<util::Stream>& stream)
    : m_stream(std::move(stream)),
      m_handle(NULL),
      m_fill(0),
      m_remaining_header(sizeof(three_hour_wav_header))
{
}

MP3Stream::~MP3Stream()
{
    TRACE << "~MP3Stream\n";
    if (m_handle)
	mpg123_delete((mpg123_handle*)m_handle);
}

unsigned MP3Stream::Read(void *buffer, size_t len, size_t *pread)
{
    if (m_remaining_header)
    {
	size_t lump = std::min(len, m_remaining_header);
	memcpy(buffer, 
	       three_hour_wav_header + sizeof(three_hour_wav_header) - m_remaining_header,
	       lump);
	*pread = lump;
	m_remaining_header -= lump;
	return 0;
    }

    if (!m_handle)
    {
	static bool inited_mpg123 = false;
	if (!inited_mpg123)
	{
	    mpg123_init();
	}
	
	m_handle = mpg123_new(NULL, NULL);
	mpg123_open_feed((mpg123_handle*)m_handle);
//	mpg123_param((mpg123_handle*)m_handle, MPG123_VERBOSE, 2, 0);
	mpg123_param((mpg123_handle*)m_handle, MPG123_ADD_FLAGS, MPG123_QUIET,
		     0);
    }

    if (m_fill == 0)
    {
	enum { INBUF = 1024 };
	unsigned char inbuf[INBUF];
	int inbufsize = 0;

	for (;;)
	{
	    size_t decoded;
	    int rc = mpg123_decode((mpg123_handle*)m_handle, 
				   inbuf, inbufsize, m_buffer, BUFSIZE,
				   &decoded);
//	    TRACE << "Fed " << inbufsize << " bytes, got back " << decoded << " bytes\n";
	    inbufsize = 0;
	    if (rc != MPG123_ERR && rc != MPG123_NEED_MORE && decoded > 0)
	    {
#if __BYTE_ORDER == __BIG_ENDIAN
		/* "audio/x-wav" is little-endian */
		for (unsigned int i=0; i<decoded; i+=2)
		{
		    unsigned char c = m_buffer[i];
		    m_buffer[i] = m_buffer[i+1];
		    m_buffer[i+1] = c;
		}
#endif
		m_fill = decoded;
		break;
	    }
	    
	    if (rc == MPG123_ERR)
		return EINVAL;

	    // Must be NEED_MORE then
	    size_t nread;
	    unsigned int ui = m_stream->Read(inbuf, INBUF, &nread);
	    if (ui != 0)
	    {
		return ui;
	    }

	    inbufsize = (int)nread;
	}
    }

    len = std::min(len, m_fill);
    memcpy(buffer, m_buffer, len);
    *pread = len;
//    TRACE << "Returning " << len << " PCM\n";
    memmove(m_buffer, m_buffer+len, m_fill-len);
    m_fill -= len;
    return 0;
}

unsigned MP3Stream::Write(const void*, size_t, size_t*)
{
    return EPERM;
}

int MP3Stream::GetHandle()
{ 
    return m_stream->GetHandle();
}

} // namespace tv

#endif // HAVE_MPG123
