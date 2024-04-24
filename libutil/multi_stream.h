#ifndef MULTI_STREAM_H
#define MULTI_STREAM_H

#include "stream.h"
#include "counted_object.h"
#include <memory>
#include <errno.h>

namespace util {

/** One-input, n-output multi-threaded sequential stream in memory.
 */
class MultiStream final: public Stream, public CountedObject
{
    class Impl;
    Impl *m_impl;

    MultiStream(std::unique_ptr<Stream>& stm, uint64_t size);
    ~MultiStream();

public:
    typedef util::CountedPointer<MultiStream> MultiStreamPtr;
    
    static MultiStreamPtr Create(std::unique_ptr<Stream>& backingstream, 
				 uint64_t size); 

    /** Create an output. You can't create any new outputs once you've
     * started Write'ing.
     */
    unsigned CreateOutput(std::unique_ptr<Stream> *pp);

    // Being a Stream (this is the input side, so no reading)
    unsigned GetStreamFlags() const override { return WRITABLE; }
    unsigned Read(void*, size_t, size_t*) override { return EACCES; }
    unsigned Write(const void *buffer, size_t len, size_t *pwrote) override;
};

typedef util::CountedPointer<MultiStream> MultiStreamPtr;

} // namespace util

#endif
