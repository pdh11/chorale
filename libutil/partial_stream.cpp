#include "partial_stream.h"
#include "counted_pointer.h"
#include "trace.h"
#include <errno.h>

namespace util {


        /* PartialSeekableStream */


class PartialSeekableStream: public SeekableStream
{
    Stream* m_stream;
    uint64_t m_begin;
    uint64_t m_end;

public:
    PartialSeekableStream(Stream*, uint64_t, uint64_t);

    // Being a SeekableStream
    unsigned int GetStreamFlags() const { return m_stream->GetStreamFlags(); }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote);
    uint64_t GetLength();
    unsigned SetLength(uint64_t);

    int GetHandle() { return m_stream->GetHandle(); }
};

PartialSeekableStream::PartialSeekableStream(Stream *s, uint64_t begin,
					     uint64_t end)
    : m_stream(s), m_begin(begin), m_end(end)
{
}

unsigned PartialSeekableStream::ReadAt(void *buffer, uint64_t pos, size_t len, 
				       size_t *pread)
{
//    TRACE << "PSRA pos=" << pos << " len=" << len << " m_end=" << m_end << "\n";
    if (pos + len + m_begin > m_end)
	len = (size_t)(m_end - pos - m_begin);
    unsigned rc = m_stream->ReadAt(buffer, pos+m_begin, len, pread);
    return rc;
}

unsigned PartialSeekableStream::WriteAt(const void *buffer, uint64_t pos, 
					size_t len, size_t *pwrote)
{
    if (pos + len + m_begin > m_end)
	len = (size_t)(m_end - pos - m_begin);
    unsigned rc = m_stream->WriteAt(buffer, pos+m_begin, len, pwrote);
    return rc;
}

uint64_t PartialSeekableStream::GetLength()
{
    return m_end - m_begin;
}

unsigned PartialSeekableStream::SetLength(uint64_t)
{
    return EINVAL;
}


        /* PartialStream */


class PartialStream: public Stream
{
    Stream* m_stream;
    unsigned long long m_length;

public:
    PartialStream(Stream*, uint64_t len);

    // Being a Stream
    unsigned int GetStreamFlags() const { return m_stream->GetStreamFlags(); }
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);

    int GetHandle() { return m_stream->GetHandle(); }
};

PartialStream::PartialStream(Stream *s, uint64_t length)
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


std::auto_ptr<Stream> CreatePartialStream(Stream *s, uint64_t begin,
					  uint64_t end)
{
    std::auto_ptr<Stream> r;

    if (s->GetStreamFlags() & Stream::SEEKABLE)
	r.reset(new PartialSeekableStream(s, begin, end));
    else
	r.reset(new PartialStream(s, end));

    return r;
}

} // namespace util

#ifdef TEST

# include "string_stream.h"

int main(int, char*[])
{
    util::StringStream ss1("ABCDEFGHIJ");

    std::auto_ptr<util::Stream> ps = util::CreatePartialStream(&ss1, 4, 8);

    util::StringStream ss2;

    unsigned int rc = CopyStream(ps.get(), &ss2);

//    TRACE << "Expect 'EFGH', got '" << ss2.str() << "'\n";

    assert(rc == 0);
    assert(ss2.str() == "EFGH");
    return 0;
}

#endif
