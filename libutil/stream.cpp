#include "stream.h"
#include "stream_test.h"
#include "memory_stream.h"
#include "trace.h"
#include <errno.h>
#include <string.h>

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

unsigned SeekableStream::WriteAllAt(const void *buffer, pos64 pos, size_t len)
{
    while (len)
    {
	size_t nwritten;
	unsigned int rc = WriteAt(buffer, pos, len, &nwritten);
	if (rc)
	    return rc;
	if (!nwritten)
	    return EIO;
	len -= nwritten;
	pos += nwritten;
	buffer = ((char*)buffer) + nwritten;
    }
    return 0;
}

unsigned SeekableStream::WriteString(const std::string& s)
{
    return WriteAll(s.c_str(), s.length());
}

unsigned SeekableStream::ReadLine(std::string *result)
{
    char buffer[1024];
    
    pos64 pos = m_pos;
    std::string s;
    
    for (;;)
    {
	size_t nread;
	unsigned rc = Read(buffer, sizeof(buffer), &nread);

	if (rc)
	    return rc;
	if (!nread)
	{
	    *result = s;
	    return 0;
	}

	char *nul = (char*)memchr(buffer, '\0', nread);
	char *lf  = (char*)memchr(buffer, '\n', nread);

	if (nul || lf)
	{
	    if (lf && (!nul || nul > lf))
		nul = lf;
	    s += std::string(buffer, nul);
	    m_pos = pos + s.length() + 1;
	    *result = s;
	    return 0;
	}

	s += std::string(buffer, buffer+nread);
    }
}


        /* Utility functions */


unsigned CopyStream(Stream *from, Stream *to)
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

unsigned CopyStream(SeekableStream *from, Stream *to)
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

unsigned DiscardStream(Stream *s)
{
    enum { BUFSIZE = 8192 };
    char buffer[BUFSIZE];

    size_t nread;
    do {
	unsigned int rc = s->Read(buffer, BUFSIZE, &nread);
	if (rc)
	{
	    TRACE << "Can't discard: " << rc << "\n";
	    return rc;
	}
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
