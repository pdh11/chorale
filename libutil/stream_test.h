#ifndef LIBUTIL_STREAM_TEST_H
#define LIBUTIL_STREAM_TEST_H

#include "stream.h"
#include <assert.h>
#include <algorithm>

namespace util {

class Scheduler;

void TestSeekableStream(Stream*);

/** Only lets data through at a dribble. Good for testing.
 */
class DribbleStream: public SeekableStream
{
    Stream *m_stm;
    size_t m_drip;

public:
    DribbleStream(Stream *stm, size_t drip)
	: m_stm(stm), m_drip(drip) {}

    // Being a SeekableStream
    unsigned GetStreamFlags() const { return m_stm->GetStreamFlags(); }
    uint64_t GetLength() { return m_stm->GetLength(); }
    unsigned SetLength(uint64_t pos) { return m_stm->SetLength(pos); }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
		    size_t *pread)
    {
	return m_stm->ReadAt(buffer, pos, std::min(len, m_drip), pread);
    }

    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len,
		     size_t *pwrote)
    {
	return m_stm->WriteAt(buffer, pos, std::min(len, m_drip), pwrote);
    }
};

#if 0
/** Lets data through at a dribble and is EWOULDBLOCK the rest of the time.
 */
class SlowStream: public Stream
{
    Scheduler *m_poller;
    size_t m_size;
    unsigned m_interval_ms;
    unsigned m_count;
    size_t m_offset;
    util::TaskCallback m_cb;

    unsigned int OnTimer();

public:
    SlowStream(Scheduler *poller,
	       size_t size, unsigned int interval_ms, unsigned int count);
    ~SlowStream();

    void Init();

    unsigned GetStreamFlags() const { return READABLE|WAITABLE; }
    unsigned Read(void *buffer, size_t len, size_t *pread);
};
#endif

} // namespace util

#endif
