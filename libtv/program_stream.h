#ifndef LIBTV_PROGRAM_STREAM_H
#define LIBTV_PROGRAM_STREAM_H 1

#include "libutil/stream.h"
#include <string>
#include <memory>

namespace tv {

/** A Stream which converts an MPEG Transport Stream into an MPEG
 * Program Stream.
 *
 * @sa http://en.wikipedia.org/wiki/Transport_Stream
 * @sa http://en.wikipedia.org/wiki/Program_stream
 */
class ProgramStream: public util::Stream
{
    std::unique_ptr<util::Stream> m_stream;
    std::string m_output_buffer;
    std::string m_video_buffer;
    std::string m_audio_buffer;
    unsigned int m_video_pid;
    unsigned int m_audio_pid;

    unsigned int FillOutputBuffer();

public:
    /** Constructs a ProgramStream.
     *
     * Note that the ProgramStream takes ownership of the underlying stream.
     */
    explicit ProgramStream(std::unique_ptr<util::Stream>&);
    ~ProgramStream();
    
    // Being a Stream
    unsigned GetStreamFlags() const { return m_stream->GetStreamFlags() & ~SEEKABLE; }
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    int GetHandle();
};

} // namespace tv

#endif
