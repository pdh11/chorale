#ifndef PARTIAL_STREAM_H
#define PARTIAL_STREAM_H 1

#include "stream.h"

namespace util {

class PartialStream: public SeekableStream
{
    SeekableStreamPtr m_stream;
    pos64 m_begin;
    pos64 m_end;

    PartialStream(SeekableStreamPtr, pos64, pos64);

public:
    static SeekableStreamPtr Create(SeekableStreamPtr underlying,
				    pos64 begin, pos64 end);

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, size_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, size_t pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

} // namespace util

#endif
