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
class AsyncWriteBuffer final: public SeekableStream
{
    class Impl;
    Impl *m_impl;

public:
    AsyncWriteBuffer(Stream *backing_stream, TaskQueue *queue);
    ~AsyncWriteBuffer();
    
    // Being a SeekableStream
    uint64_t GetLength() override;
    unsigned SetLength(uint64_t) override;
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
                    size_t *pread) override;
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote) override;

    unsigned GetStreamFlags() const override
    {
        return READABLE|WRITABLE|SEEKABLE;
    }
};

} // namespace util

#endif
