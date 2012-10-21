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
    unsigned GetStreamFlags() const { return READABLE|WRITABLE|SEEKABLE; }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote);
    uint64_t GetLength();
    unsigned SetLength(uint64_t);
};

} // namespace util

#endif
