#ifndef LIBTV_MP3_STREAM_H
#define LIBTV_MP3_STREAM_H

#include "libutil/stream.h"
#include <memory>

namespace tv {

/** A Stream that decodes an MP3 Stream into a WAV stream.
 */
class MP3Stream final: public util::Stream
{
    std::unique_ptr<util::Stream> m_stream;
    void *m_handle;
    enum { BUFSIZE = 1152*4 };
    unsigned char m_buffer[BUFSIZE];
    size_t m_fill;
    size_t m_remaining_header;

public:
    /** Constructs an MP3Stream.
     *
     * Note that the MP3Stream takes ownership of the underlying stream.
     */
    MP3Stream(std::unique_ptr<util::Stream>& stm);
    ~MP3Stream();
    
    // Being a Stream
    unsigned GetStreamFlags() const override { return m_stream->GetStreamFlags() & ~SEEKABLE; }
    unsigned Read(void *buffer, size_t len, size_t *pread) override;
    unsigned Write(const void *buffer, size_t len, size_t *pwrote) override;
    int GetHandle() override;
};

} // namespace tv

#endif
