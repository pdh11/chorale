#include "partial_stream.h"
#include "string_stream.h"
#include "trace.h"
#include <errno.h>

namespace util {

PartialStream::PartialStream(SeekableStreamPtr s, pos64 begin, pos64 end)
    : m_stream(s), m_begin(begin), m_end(end)
{
}

SeekableStreamPtr PartialStream::Create(SeekableStreamPtr s, 
					pos64 begin, pos64 end)
{
    return SeekableStreamPtr(new PartialStream(s,begin,end));
}

unsigned PartialStream::ReadAt(void *buffer, size_t pos, size_t len, 
			       size_t *pread)
{
//    TRACE << "PSRA pos=" << pos << " len=" << len << " m_end=" << m_end << "\n";
    if (pos + len + m_begin > m_end)
	len = m_end - pos - m_begin;
    unsigned rc = m_stream->ReadAt(buffer, pos+m_begin, len, pread);
    return rc;
}

unsigned PartialStream::WriteAt(const void *buffer, size_t pos, size_t len,
				size_t *pwrote)
{
    if (pos + len + m_begin > m_end)
	len = m_end - pos - m_begin;
    unsigned rc = m_stream->WriteAt(buffer, pos+m_begin, len, pwrote);
    return rc;
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

    unsigned int rc = CopyStream(ps, ss2);

//    TRACE << "Expect 'EFGH', got '" << ss2->str() << "'\n";

    assert(rc == 0);
    assert(ss2->str() == "EFGH");
    return 0;
}

#endif
