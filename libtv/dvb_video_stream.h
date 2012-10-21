#ifndef TV_DVB_VIDEO_STREAM_H
#define TV_DVB_VIDEO_STREAM_H 1

#include "libutil/stream.h"

namespace tv {

namespace dvb {

class VideoStream: public util::Stream
{
    int m_audio_fd;
    int m_video_fd;
    bool m_started;
    unsigned m_pes_packet_remaining;
    unsigned int m_fill;

    const unsigned char *m_esdata;
    unsigned int m_eslen;
    enum { BUFFER = 65536 };
    unsigned char m_buffer[BUFFER];

public:
    VideoStream();
    ~VideoStream();

    unsigned int Open(unsigned int adaptor, unsigned int device,
		      unsigned int audiopid, unsigned int videopid);

    // being a Stream
    unsigned GetStreamFlags() const { return READABLE|POLLABLE; }
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    int GetHandle() { return m_audio_fd; }
};

} // namespace dvb

} // namespace tv

#endif
