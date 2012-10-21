#include "stream.h"
#include "stream_test.h"
#include "memory_stream.h"
#include "trace.h"
#include <errno.h>

namespace util {


        /* Stream */


unsigned Stream::WriteAll(const void *buffer, size_t len)
{
    while (len)
    {
	size_t wrote;
	unsigned rc = Write(buffer, len, &wrote);
	if (rc)
	    return rc;
	if (!wrote)
	    return EIO;
	len -= wrote;
	buffer = ((const char*)buffer) + wrote;
    }

    return 0;
}

unsigned Stream::ReadAll(void *buffer, size_t len)
{
    while (len)
    {
	size_t nread;
	unsigned rc = Read(buffer, len, &nread);
	if (rc)
	    return rc;
	if (!nread)
	    return EIO;
	len -= nread;
	buffer = ((char*)buffer) + nread;
    }

    return 0;
}


        /* SeekableStream */


SeekableStream::SeekableStream()
    : m_pos(0)
{
}

unsigned SeekableStream::Read(void *buffer, size_t len, size_t *pread)
{
    size_t nread;
    unsigned int rc = ReadAt(buffer, m_pos, len, &nread);
    if (rc == 0)
    {
	m_pos += nread;
	*pread = nread;
    }
    return rc;
}

unsigned SeekableStream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    size_t nwrote;
    unsigned int rc = WriteAt(buffer, m_pos, len, &nwrote);
    if (rc == 0)
    {
	m_pos += nwrote;
	*pwrote = nwrote;
    }
    return rc;
}


        /* Utility functions */


unsigned CopyStream(StreamPtr from, StreamPtr to)
{
    enum { BUFSIZE = 8192 };
    char buffer[BUFSIZE];

    size_t nread;
    do {
	nread = 0;
	unsigned int rc = from->Read(buffer, BUFSIZE, &nread);
	if (rc)
	    return rc;
	rc = to->WriteAll(buffer, nread);
	if (rc)
	    return rc;
    } while (nread);

    return 0;
}

unsigned CopyStream(SeekableStreamPtr from, StreamPtr to)
{
    SeekableStream::pos64 pos = 0;
    enum { BUFSIZE = 8192 };
    char buffer[BUFSIZE];

    size_t nread;
    do {
	nread = 0;
	unsigned int rc = from->ReadAt(buffer, pos, BUFSIZE, &nread);
//	TRACE << "rc=" << rc << " nread=" << nread << "\n";
	if (rc)
	    return rc;
	rc = to->WriteAll(buffer, nread);
	if (rc)
	    return rc;
	pos += nread;
    } while (nread);

    return 0;
}

} // namespace util

#ifdef TEST

int main()
{
    util::MemoryStreamPtr msp;

    unsigned int rc = util::MemoryStream::Create(&msp);
    assert(rc == 0);

    TestSeekableStream(msp);

    return 0;
}

#endif
