#include "string_stream.h"

namespace util {

StringStream::StringStream()
    : m_pos(0)
{
}

StringStream::~StringStream()
{
}

StringStreamPtr StringStream::Create()
{
    return StringStreamPtr(new StringStream);
}

unsigned StringStream::Read(void *buffer, size_t len, size_t *pread)
{
    size_t remain = m_string.length() - m_pos;
    size_t nread = std::min(len, remain);
    memcpy(buffer, m_string.c_str() + m_pos, nread);
    *pread = nread;
    m_pos += nread;
    return 0;
}

unsigned StringStream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    m_string.replace(m_pos, len, (const char*)buffer, len);
    *pwrote = len;
    m_pos += len;
    return 0;
}

void StringStream::Seek(pos64 pos)
{
    m_pos = std::min((std::string::size_type)pos, m_string.length());
}

SeekableStream::pos64 StringStream::Tell()
{
    return m_pos;
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
	if (m_pos > len)
	    m_pos = len;
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
