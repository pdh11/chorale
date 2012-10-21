#ifndef STREAM_H
#define STREAM_H

#include "attributes.h"
#include <string>
#include <stdint.h>

namespace util {

template <class T> class CountedPointer;
template <class T> class PtrCallback;
class Task;
typedef CountedPointer<Task> TaskPtr;
typedef PtrCallback<TaskPtr> TaskCallback;

enum { NOT_POLLABLE = -1 };

/** Abstract base class for anything which can be streamed.
 */
class Stream
{
public:
    virtual ~Stream() {}

    enum {
	READABLE =  1, ///< Supports Read()
	WRITABLE =  2, ///< Supports Write()
	WAITABLE =  4, ///< Supports Wait()
	SEEKABLE =  8, ///< Supports Seek()/Tell()
	POLLABLE = 16  ///< Supports GetHandle()
    };
    virtual unsigned int GetStreamFlags() const = 0;

    /** Returns 0 for success or an errno for failure. EOF is not a failure.
     *
     * If result is 0, *pread is 0 iff (len == 0 or EOF).
     * 
     * Streams that are neither WAITABLE nor POLLABLE should never return
     * EWOULDBLOCK.
     *
     * @pre GetStreamFlags() & READABLE
     */
    virtual unsigned Read(void *buffer, size_t len, size_t *pread)
	ATTRIBUTE_WARNUNUSED;

    /** Returns 0 for success or an errno for failure.
     *
     * EOF (e.g. on a socket) is an error.
     * 
     * Streams that are neither WAITABLE nor POLLABLE should never return
     * EWOULDBLOCK.
     *
     * @pre GetStreamFlags() & WRITABLE
     */
    virtual unsigned Write(const void *buffer, size_t len, size_t *pwrote)
	ATTRIBUTE_WARNUNUSED;

    /** Perform a seek within the stream.
     *
     * The next Read() or Write() operation will start at this offset.
     * Non-seekable streams should return ESPIPE.
     *
     * @pre GetStreamFlags() & SEEKABLE
     */
    virtual unsigned int Seek(uint64_t pos);

    /** Return the current offset within the stream.
     *
     * Only makes sense for seekable streams.
     *
     * @pre GetStreamFlags() & SEEKABLE
     */
    virtual uint64_t Tell();

    /** Return the current length of the stream.
     *
     * Only makes sense for seekable streams.
     *
     * @pre GetStreamFlags() & SEEKABLE
     */
    virtual uint64_t GetLength();

    /** Set the length of the stream.
     *
     * Only makes sense for seekable streams.
     *
     * @pre GetStreamFlags() & SEEKABLE
     * @pre GetStreamFlags() & WRITABLE
     */
    virtual unsigned SetLength(uint64_t);

    /** Atomic seek-and-read.
     *
     * May (or may not) change seek position. Returns ESPIPE for
     * non-seekable streams.
     *
     * @pre GetStreamFlags() & SEEKABLE
     * @pre GetStreamFlags() & READABLE
     */
    virtual unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
			    size_t *pread) ATTRIBUTE_WARNUNUSED;

    /** Atomic seek-and-write.
     *
     * May (or may not) change seek position. Returns ESPIPE for
     * non-seekable streams.
     *
     * @pre GetStreamFlags() & SEEKABLE
     * @pre GetStreamFlags() & WRITABLE
     */
    virtual unsigned WriteAt(const void *buffer, uint64_t pos, size_t len,
			     size_t *pwrote) ATTRIBUTE_WARNUNUSED;
    
    /** Get the underlying poll handle for the stream.
     *
     * @return Handle suitable for Scheduler::WaitForReadable, or -1 if none.
     *
     * @todo Replace usage of this, with Wait().
     *
     * @pre GetStreamFlags() & POLLABLE
     */
    virtual int GetHandle() { return NOT_POLLABLE; }

    /** Wait for the stream to become ready, then call the callback.
     *
     * There is no guarantee as to what thread the callback is made on; it
     * could be a high-priority poller or a UI thread, so if you're going to
     * do lots of work, or if you're going to block, punt yourself to a
     * background thread in your Wait callback.
     *
     * Default implementation does nothing and returns an ENOSYS error.
     *
     * @pre GetStreamFlags() & WAITABLE
     */
    virtual unsigned Wait(const TaskCallback& callback);

    
    /* The following helper functions could be implemented using the
     * class interface, and are members for notational reasons only.
     */
    

    /** Returns 0 for success or an errno if all bytes not read. Premature
     * EOF is a failure, returning ENODATA.
     *
     * @pre GetStreamFlags() & READABLE
     */
    unsigned ReadAll(void *buffer, size_t len) ATTRIBUTE_WARNUNUSED;

    /** Reads until newline, zero byte, or EOF. Does not return the
     * terminator.
     *
     * Note that this ONLY works on SEEKABLE streams, because it sets the
     * file position back to point just after the terminator.
     *
     * @pre GetStreamFlags() & SEEKABLE
     * @pre GetStreamFlags() & READABLE
     */
    unsigned ReadLine(std::string *line) ATTRIBUTE_WARNUNUSED;

    /** Returns 0 for success or an errno if all bytes not written. Premature
     * EOF is a failure, returning EIO.
     *
     * @pre GetStreamFlags() & WRITABLE
     */
    unsigned WriteAll(const void *buffer, size_t len) ATTRIBUTE_WARNUNUSED;

    /** Does not add a newline.
     *
     * @pre GetStreamFlags() & WRITABLE
     */
    unsigned WriteString(const std::string&) ATTRIBUTE_WARNUNUSED;

    /** Write the whole of a block of data.
     *
     * Returns 0 for success or an errno if all bytes not
     * written. Premature EOF is a failure, returning EIO. Returns
     * ESPIPE for non-seekable streams.
     *
     * @pre GetStreamFlags() & WRITABLE
     * @pre GetStreamFlags() & SEEKABLE
     */
    unsigned WriteAllAt(const void *buffer, uint64_t pos, 
			size_t len) ATTRIBUTE_WARNUNUSED;
};

/** Abstract base class for anything which can be streamed, and which is
 * also seekable.
 */
class SeekableStream: public Stream
{
    uint64_t m_pos;

public:
    SeekableStream();

    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    unsigned int Seek(uint64_t pos) { m_pos = pos; return 0; }
    uint64_t Tell() { return m_pos; }

    virtual uint64_t GetLength() = 0;
    virtual unsigned SetLength(uint64_t) = 0;

    /** Atomic seek-and-read.
     *
     * May (or may not) change seek position.
     */
    virtual unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
			    size_t *pread) ATTRIBUTE_WARNUNUSED = 0;

    /** Atomic seek-and-write.
     *
     * May (or may not) change seek position.
     */
    virtual unsigned WriteAt(const void *buffer, uint64_t pos, size_t len,
			     size_t *pwrote) ATTRIBUTE_WARNUNUSED = 0;
};

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
