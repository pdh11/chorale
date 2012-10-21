#include "partial_stream.h"
#include "string_stream.h"
#include <errno.h>

namespace util {

PartialStream::PartialStream(SeekableStreamPtr s, pos64 begin, pos64 end)
    : m_stream(s), m_begin(begin), m_end(end), m_pos(begin)
{
    s->Seek(begin);
}

SeekableStreamPtr PartialStream::Create(SeekableStreamPtr s, 
					pos64 begin, pos64 end)
{
    return SeekableStreamPtr(new PartialStream(s,begin,end));
}

unsigned PartialStream::Read(void *buffer, size_t len, size_t *pread)
{
    if (m_pos + len > m_end)
	len = m_end - m_pos;
    unsigned rc = m_stream->Read(buffer, len, pread);
    if (rc == 0)
	m_pos += *pread;
    return rc;
}

unsigned PartialStream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    if (m_pos + len > m_end)
	len = m_end - m_pos;
    unsigned rc = m_stream->Write(buffer, len, pwrote);
    if (rc == 0)
	m_pos += *pwrote;
    return rc;
}

void PartialStream::Seek(pos64 pos)
{
    m_pos = m_begin + pos;
    if (m_pos > m_end)
	m_pos = m_end;
    m_stream->Seek(m_pos);
}

SeekableStream::pos64 PartialStream::Tell()
{
    return m_pos - m_begin;
}

SeekableStream::pos64 PartialStream::GetLength()
{
    return m_end - m_begin;
}

unsigned PartialStream::SetLength(pos64)
{
    return EINVAL;
}

} // namespace util

#ifdef TEST

int main(int, char*[])
{
    util::StringStreamPtr ss1 = util::StringStream::Create();
    ss1->str() = "ABCDEFGHIJ";

    util::SeekableStreamPtr ps = util::PartialStream::Create(ss1, 4, 8);

    util::StringStreamPtr ss2 = util::StringStream::Create();

    unsigned int rc = ss2->Copy(ps);

    assert(rc == 0);
    assert(ss2->str() == "EFGH");
    return 0;
}

#endif
