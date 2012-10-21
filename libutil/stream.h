#ifndef STREAM_H
#define STREAM_H

#include "attributes.h"
#include "counted_object.h"

namespace util {

/** Abstract base class for anything which can be streamed.
 */
class Stream: public CountedObject
{
public:
    /** Returns 0 for success or an errno for failure. EOF is not a failure.
     */
    virtual unsigned Read(void *buffer, size_t len, size_t *pread) 
	ATTRIBUTE_WARNUNUSED = 0;

    virtual unsigned Write(const void *buffer, size_t len, size_t *pwrote)
	ATTRIBUTE_WARNUNUSED = 0;

    /** Returns 0 for success or an errno if all bytes not read. Premature
     * EOF is a failure, returning ENODATA.
     */
    unsigned ReadAll(void *buffer, size_t len) ATTRIBUTE_WARNUNUSED;

    /** Returns 0 for success or an errno if all bytes not written. Premature
     * EOF is a failure, returning EIO.
     */
    unsigned WriteAll(const void *buffer, size_t len) ATTRIBUTE_WARNUNUSED;

    typedef boost::intrusive_ptr<Stream> StreamPtr;

    /** Repeatedly Read()s the other stream and Writes() this, until EOF.
     */
    unsigned Copy(StreamPtr other);
};

typedef boost::intrusive_ptr<Stream> StreamPtr;


/** Abstract base class for anything which can be streamed, and which is
 * also seekable.
 */
class SeekableStream: public Stream
{
public:
    typedef unsigned long long pos64;

    virtual void Seek(pos64 pos) = 0;
    virtual pos64 Tell() = 0;
    virtual pos64 GetLength() = 0;
    virtual unsigned SetLength(pos64) = 0;
};

typedef boost::intrusive_ptr<SeekableStream> SeekableStreamPtr;

} // namespace util

#endif
