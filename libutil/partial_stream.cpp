#include "partial_stream.h"
#include "counted_pointer.h"
#include "trace.h"
#include <errno.h>

namespace util {


        /* PartialSeekableStream */


class PartialSeekableStream: public SeekableStream
{
    SeekableStreamPtr m_stream;
    pos64 m_begin;
    pos64 m_end;

public:
    PartialSeekableStream(SeekableStreamPtr, pos64, pos64);

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);

    util::PollHandle GetHandle() { return m_stream->GetHandle(); }
};

PartialSeekableStream::PartialSeekableStream(SeekableStreamPtr s, pos64 begin,
					     pos64 end)
    : m_stream(s), m_begin(begin), m_end(end)
{
}

unsigned PartialSeekableStream::ReadAt(void *buffer, pos64 pos, size_t len, 
				       size_t *pread)
{
//    TRACE << "PSRA pos=" << pos << " len=" << len << " m_end=" << m_end << "\n";
    if (pos + len + m_begin > m_end)
	len = (size_t)(m_end - pos - m_begin);
    unsigned rc = m_stream->ReadAt(buffer, pos+m_begin, len, pread);
    return rc;
}

unsigned PartialSeekableStream::WriteAt(const void *buffer, pos64 pos, 
					size_t len, size_t *pwrote)
{
    if (pos + len + m_begin > m_end)
	len = (size_t)(m_end - pos - m_begin);
    unsigned rc = m_stream->WriteAt(buffer, pos+m_begin, len, pwrote);
    return rc;
}

SeekableStream::pos64 PartialSeekableStream::GetLength()
{
    return m_end - m_begin;
}

unsigned PartialSeekableStream::SetLength(pos64)
{
    return EINVAL;
}


        /* PartialStream */


class PartialStream: public Stream
{
    StreamPtr m_stream;
    unsigned long long m_length;

public:
    PartialStream(StreamPtr, unsigned long long len);

    // Being a Stream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    util::PollHandle GetHandle() { return m_stream->GetHandle(); }
};

PartialStream::PartialStream(StreamPtr s, unsigned long long length)
    : m_stream(s), m_length(length)
{
}

unsigned PartialStream::Read(void *buffer, size_t len, size_t *pread)
{
    if (len > m_length)
	len = (size_t)m_length;
    if (!len)
    {
	*pread = 0;
	return 0;
    }

//    TRACE << "Reading " << len << "\n";
    unsigned rc = m_stream->Read(buffer, len, pread);
//    TRACE << "Read returned " << rc << " pread=" << *pread << "\n";
    if (rc == 0)
	m_length -= *pread;
    return rc;
}

unsigned PartialStream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    if (len > m_length)
	len = (size_t)m_length;
    unsigned rc = m_stream->Write(buffer, len, pwrote);
    if (rc == 0)
	m_length -= *pwrote;
    return rc;
}


        /* Exposed functions */


StreamPtr CreatePartialStream(StreamPtr s, unsigned long long length)
{
    return StreamPtr(new PartialStream(s, length));
}

SeekableStreamPtr CreatePartialStream(SeekableStreamPtr s, 
				      unsigned long long begin,
				      unsigned long long end)
{
    return SeekableStreamPtr(new PartialSeekableStream(s,begin,end));
}

} // namespace util

#ifdef TEST

# include "string_stream.h"

int main(int, char*[])
{
    util::StringStreamPtr ss1 = util::StringStream::Create();
    ss1->str() = "ABCDEFGHIJ";

    util::SeekableStreamPtr ps = util::CreatePartialStream(ss1, 4, 8);

    util::StringStreamPtr ss2 = util::StringStream::Create();

    unsigned int rc = CopyStream(ps.get(), ss2.get());

//    TRACE << "Expect 'EFGH', got '" << ss2->str() << "'\n";

    assert(rc == 0);
    assert(ss2->str() == "EFGH");
    return 0;
}

#endif
