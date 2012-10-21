#include "string_stream.h"
#include "counted_pointer.h"
#include <string.h>

namespace util {

StringStream::StringStream()
{
}

StringStream::StringStream(const std::string& s)
    : m_string(s)
{
}

StringStream::~StringStream()
{
}

unsigned StringStream::ReadAt(void *buffer, uint64_t pos, size_t len,
			      size_t *pread)
{
    size_t remain = m_string.length() - (size_t)pos;
    size_t nread = std::min(len, remain);
    memcpy(buffer, m_string.c_str() + pos, nread);
    *pread = nread;
    return 0;
}

unsigned StringStream::WriteAt(const void *buffer, uint64_t pos, size_t len, 
			       size_t *pwrote)
{
    m_string.replace((size_t)pos, len, (const char*)buffer, len);
    *pwrote = len;
    return 0;
}

uint64_t StringStream::GetLength()
{
    return m_string.length();
}

unsigned StringStream::SetLength(uint64_t len)
{
    if (len > m_string.length())
	m_string.append((size_t)len - m_string.length(), ' ');
    else
    {
	m_string.erase((size_t)len);
	if (Tell() > len)
	    Seek(len);
    }
    return 0;
}

} // namespace util

#ifdef TEST

# include "stream_test.h"

int main()
{
    util::StringStream msp;

    TestSeekableStream(&msp);

    return 0;
}

#endif
