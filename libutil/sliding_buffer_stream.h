#ifndef LIBUTIL_SLIDING_BUFFER_STREAM_H
#define LIBUTIL_SLIDING_BUFFER_STREAM_H 1

#include "locking.h"
#include "stream.h"
#include <boost/thread/mutex.hpp>

namespace util {

/** A SeekableStream that only remembers a finite suffix of the data
 * written to it.
 */
class SlidingBufferStream: public SeekableStream
{    
    boost::mutex m_mutex;

    /** Circular data buffer */
    unsigned char *m_buffer;

    /** Allocated size of m_buffer */
    size_t m_buffer_size;

    /** Bytes used in m_buffer */
    size_t m_buffer_fill;

    /** Read-offset in circular buffer */
    size_t m_buffer_offset;

    /** Offset in the stream corresponding to m_buffer_offset */
    pos64 m_stream_offset;

    /** Pretended length of the stream */
    pos64 m_stream_length;

public:
    explicit SlidingBufferStream(size_t size);
    ~SlidingBufferStream();

    // Being a SeekableStream
    pos64 GetLength();
    unsigned SetLength(pos64);
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
};

/** A SlidingBufferStream that eagerly reads from another stream.
 */
class EagerSlidingBufferStream: public SlidingBufferStream
{
    boost::mutex m_mutex2; ///< @todo fold this into the SBS mutex
    StreamPtr m_upstream;
    pos64 m_writepos;

public:
    EagerSlidingBufferStream(size_t buffer_size, StreamPtr upstream);

    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);

    util::PollHandle GetReadHandle();
};

} // namespace util

#endif
