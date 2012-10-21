#ifndef ASYNC_WRITE_BUFFER_H
#define ASYNC_WRITE_BUFFER_H

#include "stream.h"

namespace util {

class TaskQueue;

/** Buffers writes into big lumps and does them on a background thread
 */
class AsyncWriteBuffer: public SeekableStream
{
    class Impl;
    Impl *m_impl;

    explicit AsyncWriteBuffer(SeekableStreamPtr, TaskQueue *queue);
    ~AsyncWriteBuffer();

public:
    static unsigned Create(SeekableStreamPtr backingstream,
			   TaskQueue *queue,
			   SeekableStreamPtr *result)
	ATTRIBUTE_WARNUNUSED;
    
    // Being a SeekableStream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    void Seek(pos64 pos);
    pos64 Tell();
    pos64 GetLength();
    unsigned SetLength(pos64);
};

}; // namespace util

#endif
