#ifndef LIBUTIL_STREAM_TEST_H
#define LIBUTIL_STREAM_TEST_H

#include "stream.h"
#include <assert.h>

namespace util {

void TestSeekableStream(SeekableStreamPtr);

/** Only lets data through at a dribble. Good for testing.
 */
class DribbleStream: public SeekableStream
{
    SeekableStreamPtr m_stm;
    size_t m_drip;

    DribbleStream(SeekableStreamPtr stm, size_t drip)
	: m_stm(stm), m_drip(drip) {}

public:
    static SeekableStreamPtr Create(SeekableStreamPtr stm, size_t drip)
    {
	assert(drip>0);
	return SeekableStreamPtr(new DribbleStream(stm, drip));
    }

    // Being a SeekableStream
    pos64 GetLength() { return m_stm->GetLength(); }
    unsigned SetLength(pos64 pos) { return m_stm->SetLength(pos); }
    unsigned ReadAt(void *buffer, pos64 pos, size_t len,
		    size_t *pread)
    {
	return m_stm->ReadAt(buffer, pos, std::min(len, m_drip), pread);
    }

    unsigned WriteAt(const void *buffer, pos64 pos, size_t len,
		     size_t *pwrote)
    {
	return m_stm->WriteAt(buffer, pos, std::min(len, m_drip), pwrote);
    }
};

} // namespace util

#endif

