#ifndef ASYNC_WRITE_BUFFER_H
#define ASYNC_WRITE_BUFFER_H

#include "stream.h"

namespace util {

class TaskQueue;

/** Buffers writes into big lumps and does them on a background thread
 */
class AsyncWriteBuffer: public Stream
{
    class Impl;
    Impl *m_impl;

    explicit AsyncWriteBuffer(StreamPtr, TaskQueue *queue);
    ~AsyncWriteBuffer();

public:
    static unsigned Create(StreamPtr backingstream,
			   TaskQueue *queue,
			   StreamPtr *result)
	ATTRIBUTE_WARNUNUSED;
    
    // Being a Stream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
};

} // namespace util

#endif
