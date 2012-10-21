#include "string_stream.h"
#include <string.h>

namespace util {

StringStream::StringStream()
{
}

StringStream::~StringStream()
{
}

StringStreamPtr StringStream::Create()
{
    return StringStreamPtr(new StringStream);
}

unsigned StringStream::ReadAt(void *buffer, pos64 pos, size_t len,
			      size_t *pread)
{
    size_t remain = m_string.length() - pos;
    size_t nread = std::min(len, remain);
    memcpy(buffer, m_string.c_str() + pos, nread);
    *pread = nread;
    return 0;
}

unsigned StringStream::WriteAt(const void *buffer, pos64 pos, size_t len, 
			       size_t *pwrote)
{
    m_string.replace(pos, len, (const char*)buffer, len);
    *pwrote = len;
    return 0;
}

SeekableStream::pos64 StringStream::GetLength()
{
    return m_string.length();
}

unsigned StringStream::SetLength(pos64 len)
{
    if (len > m_string.length())
	m_string.append(len - m_string.length(), ' ');
    else
    {
	m_string.erase(len);
	if (Tell() > len)
	    Seek(len);
    }
    return 0;
}

} // namespace util

#ifdef TEST

#include "stream_test.h"

int main()
{
    util::StringStreamPtr msp = util::StringStream::Create();
    assert(msp);

    TestSeekableStream(msp);

    return 0;
}

#endif
