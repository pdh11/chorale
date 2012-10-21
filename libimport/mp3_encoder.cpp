#include "mp3_encoder.h"
#include "config.h"
#include "libutil/trace.h"
#include "libutil/stream.h"
#include <errno.h>

#if HAVE_LAME
#include <lame/lame.h>

namespace import {

MP3Encoder::MP3Encoder()
    : m_lame(NULL)
{
}

MP3Encoder::~MP3Encoder()
{
    if (m_lame)
    {
	lame_close((lame_t)m_lame);
	m_lame = NULL;
    }
}

unsigned int MP3Encoder::Init(util::Stream *output, size_t)
{
    m_output = output;
    m_lame = lame_init();

    int rc = lame_set_preset((lame_t)m_lame, STANDARD);
    if (rc<0)
    {
	TRACE << "lame_set_preset returned " << rc << "\n";
	return EINVAL;
    }
    rc = lame_init_params((lame_t)m_lame);
    if (rc<0)
    {
	TRACE << "lame_init_params returned " << rc << "\n";
	return EINVAL;
    }

    return 0;
}

unsigned int MP3Encoder::Encode(const short *samples, size_t sample_pairs)
{
    unsigned char outbuffer[16*1024];

    int rc = lame_encode_buffer_interleaved((lame_t)m_lame,
					    const_cast<short*>(samples),
					    (int)sample_pairs, outbuffer,
					    (int)sizeof(outbuffer));
    if (rc < 0)
    {
	TRACE << "Lame encode failed: " << rc << "\n";
	return EINVAL;
    }

    return m_output->WriteAll(outbuffer, rc);
}

unsigned int MP3Encoder::Finish()
{
    unsigned char outbuffer[32*1024];

    int rc = lame_encode_flush((lame_t)m_lame, outbuffer, sizeof(outbuffer));
    if (rc < 0)
    {
	TRACE << "Lame flush failed: " << rc << "\n";
	return EINVAL;
    }

    unsigned int rc2 = m_output->WriteAll(outbuffer, rc);
    if (rc2)
	return rc2;

#if HAVE_LAME_GET_LAMETAG_FRAME
    size_t sz = lame_get_lametag_frame((lame_t)m_lame, outbuffer,
				       sizeof(outbuffer));
    
    rc2 = m_output->WriteAllAt(outbuffer, 0, sz);
    if (rc2)
	return rc2;

    lame_close((lame_t)m_lame);
    m_lame = NULL;
#endif
    return 0;
}

unsigned int MP3Encoder::PostFinish(const std::string& filename)
{
#if !HAVE_LAME_GET_LAMETAG_FRAME
    FILE *f = fopen(filename.c_str(), "rw");
    if (f)
    {
	lame_mp3_tags_fid((lame_t)m_lame, f);
	fclose(f);
    }
#else
    std::string s(filename);
#endif
    return 0;
}

} // namespace import

#endif // HAVE_LAME
