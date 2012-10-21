#ifndef ASYNC_WRITE_BUFFER_H
#define ASYNC_WRITE_BUFFER_H

#include "stream.h"

namespace util {

class TaskQueue;

/** Buffers writes into big lumps and does them on a background thread
 *
 * Non-streaming (non-consecutive) writes still work, but are less efficient
 * (probably synchronous).
 */
class AsyncWriteBuffer: public SeekableStream
{
    class Impl;
    Impl *m_impl;

public:
    AsyncWriteBuffer(Stream *backing_stream, TaskQueue *queue);
    ~AsyncWriteBuffer();
    
    // Being a SeekableStream
    uint64_t GetLength();
    unsigned SetLength(uint64_t);
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote);

    unsigned GetStreamFlags() const { return READABLE|WRITABLE|SEEKABLE; }
};

} // namespace util

#endif
