#ifndef PARTIAL_STREAM_H
#define PARTIAL_STREAM_H 1

#include "stream.h"

namespace util {

class PartialStream: public SeekableStream
{
    SeekableStreamPtr m_stream;
    pos64 m_begin;
    pos64 m_end;
    pos64 m_pos;

    PartialStream(SeekableStreamPtr, pos64, pos64);

public:
    static SeekableStreamPtr Create(SeekableStreamPtr underlying,
				    pos64 begin, pos64 end);

    // Being a Stream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    // Being a SeekableStream
    void Seek(pos64 pos);
    pos64 Tell();
    pos64 GetLength();
    unsigned SetLength(pos64);
};

} // namespace util

#endif
