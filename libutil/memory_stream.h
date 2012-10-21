#ifndef MEMORY_STREAM_H
#define MEMORY_STREAM_H

#include "stream.h"
#include <boost/noncopyable.hpp>

namespace util {

template <class T> class CountedPointer;

/** A SeekableStream kept entirely in memory.
 *
 * Optimised for big streams -- allocates memory in 1Mb chunks!
 */
class MemoryStream: public SeekableStream, private boost::noncopyable
{
    class Impl;
    Impl *m_impl;

    explicit MemoryStream(size_t sizeHint = 0);
    ~MemoryStream();

public:
    typedef util::CountedPointer<MemoryStream> MemoryStreamPtr;

    static unsigned Create(MemoryStreamPtr*, size_t sizeHint = 0);

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

typedef util::CountedPointer<MemoryStream> MemoryStreamPtr;

} // namespace util

#endif
