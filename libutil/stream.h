#ifndef STREAM_H
#define STREAM_H

#include "attributes.h"
#include "counted_object.h"
#include "pollable.h"
#include <string>

namespace util {

template <class T> class CountedPointer;

/** Abstract base class for anything which can be streamed.
 */
class Stream: public CountedObject<>, public Pollable
{
public:
    /** Returns 0 for success or an errno for failure. EOF is not a failure.
     *
     * If result is 0, *pread is 0 iff (len == 0 or EOF).
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
};

typedef CountedPointer<Stream> StreamPtr;

/** Abstract base class for anything which can be streamed, and which is
 * also seekable.
 */
class SeekableStream: public Stream
{
public:
    typedef unsigned long long pos64;

private:
    pos64 m_pos;

protected:
    SeekableStream();

public:
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    void Seek(pos64 pos) { m_pos = pos; }
    pos64 Tell() { return m_pos; }

    virtual pos64 GetLength() = 0;
    virtual unsigned SetLength(pos64) = 0;

    /** Atomic seek-and-read.
     *
     * May (or may not) change seek position.
     */
    virtual unsigned ReadAt(void *buffer, pos64 pos, size_t len,
			    size_t *pread) ATTRIBUTE_WARNUNUSED = 0;

    /** Atomic seek-and-write.
     *
     * May (or may not) change seek position.
     */
    virtual unsigned WriteAt(const void *buffer, pos64 pos, size_t len,
			     size_t *pwrote) ATTRIBUTE_WARNUNUSED = 0;

    /** Returns 0 for success or an errno if all bytes not written. Premature
     * EOF is a failure, returning EIO.
     */
    unsigned WriteAllAt(const void *buffer, pos64 pos, 
			size_t len) ATTRIBUTE_WARNUNUSED;

    /** Does not add a '\n'.
     */
    unsigned WriteString(const std::string&) ATTRIBUTE_WARNUNUSED;

    /** Reads until '\n', '\0', or EOF. Does not return the terminator.
     */
    unsigned ReadLine(std::string *line) ATTRIBUTE_WARNUNUSED;
};

typedef CountedPointer<SeekableStream> SeekableStreamPtr;

/** Repeatedly Read()s one stream and Writes() the other, until EOF or error.
 */
unsigned int CopyStream(Stream* from, Stream* to);

/** Repeatedly Read()s one stream and Writes() the other, until EOF or error.
 *
 * Uses ReadAt() so is safe to use several times independently on the same
 * stream.
 */
unsigned int CopyStream(SeekableStream* from, Stream* to);

/** Repeatedly Read()s stream, discarding data, until EOF or error.
 */
unsigned int DiscardStream(Stream*);

} // namespace util

#endif
