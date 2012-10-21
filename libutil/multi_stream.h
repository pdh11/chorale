#ifndef MULTI_STREAM_H
#define MULTI_STREAM_H

#include "stream.h"
#include <errno.h>

namespace util {

/** One-input, n-output multi-threaded sequential stream in memory.
 */
class MultiStream: public Stream
{
    class Impl;
    Impl *m_impl;
    MultiStream(SeekableStreamPtr, SeekableStream::pos64);
    ~MultiStream();

public:
    typedef boost::intrusive_ptr<MultiStream> MultiStreamPtr;
    
    static unsigned Create(SeekableStreamPtr backingstream, 
			   SeekableStream::pos64 size,
			   MultiStreamPtr *result) ATTRIBUTE_WARNUNUSED; 

    /** Create an output. You can't create any new outputs once you've
     * started Write'ing.
     */
    unsigned CreateOutput(StreamPtr *pp);

    // Being a Stream (this is the input side, so no reading)
    unsigned Read(void*, size_t, size_t*) { return EACCES; }
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
};

typedef boost::intrusive_ptr<MultiStream> MultiStreamPtr;

} // namespace util

#endif
