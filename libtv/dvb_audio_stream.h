#ifndef TV_DVB_AUDIO_STREAM_H
#define TV_DVB_AUDIO_STREAM_H 1

#include "libutil/stream.h"

namespace tv {

namespace dvb {

class AudioStream: public util::Stream
{
    int m_fd;
    bool m_started;
    unsigned m_pes_packet_remaining;
    enum { BUFFER = 8192 };
    unsigned char m_buffer[BUFFER];
    unsigned int m_fill;

    const unsigned char *m_esdata;
    unsigned int m_eslen;

public:
    AudioStream();
    ~AudioStream();

    unsigned int Open(unsigned int adaptor, unsigned int device,
		      unsigned int pid);

    // being a Stream
    unsigned GetStreamFlags() const { return READABLE|POLLABLE; }
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    int GetHandle();
};

} // namespace dvb

} // namespace tv

#endif
