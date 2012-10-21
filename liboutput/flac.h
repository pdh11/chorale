#ifndef OUTPUT_FLAC_H
#define OUTPUT_FLAC_H 1

#include "libutil/stream.h"

namespace output {

class FlacStream: public util::SeekableStream
{
    class Impl;
    Impl *m_impl;

public:
    explicit FlacStream(util::SeekableStreamPtr upstream);
    ~FlacStream();

    // Being a SeekableStream
    virtual unsigned Read(void *buffer, size_t len, size_t *pread);
    virtual unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    virtual void Seek(pos64 pos);
    virtual pos64 Tell();
    virtual pos64 GetLength();
    virtual unsigned SetLength(pos64);
};

};

#endif
