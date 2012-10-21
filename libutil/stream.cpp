#include "stream.h"
#include "stream_test.h"
#include "memory_stream.h"
#include <errno.h>

namespace util {

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
	    return ENODATA;
	len -= nread;
	buffer = ((char*)buffer) + nread;
    }

    return 0;
}

unsigned Stream::Copy(StreamPtr other)
{
    enum { BUFSIZE = 8192 };
    char buffer[BUFSIZE];

    size_t nread;
    do {
	nread = 0;
	unsigned int rc = other->Read(buffer, BUFSIZE, &nread);
	if (rc)
	    return rc;
	rc = WriteAll(buffer, nread);
	if (rc)
	    return rc;
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
