#ifndef MEMORY_STREAM_H
#define MEMORY_STREAM_H

#include "stream.h"
#include <memory>

namespace util {

/** A SeekableStream kept entirely in memory.
 *
 * Optimised for big streams -- allocates memory in 1Mb chunks!
 */
class MemoryStream: public SeekableStream
{
    class Impl;
    Impl *m_impl;

public:
    explicit MemoryStream(size_t sizeHint = 0);
    ~MemoryStream();

    // Being a SeekableStream
    unsigned GetStreamFlags() const override
    {
        return READABLE|WRITABLE|SEEKABLE;
    }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
                    size_t *pread) override;
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote) override;
    uint64_t GetLength() override;
    unsigned SetLength(uint64_t) override;
};

} // namespace util

#endif
