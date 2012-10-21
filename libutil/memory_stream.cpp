#include "memory_stream.h"
#include "stream_test.h"
#include "trace.h"
#include <string.h>

namespace util {

class MemoryStream::Impl
{
public:
    struct Chunk: private boost::noncopyable
    {
	size_t len;
	void *data;

	explicit Chunk(size_t len);
	~Chunk();
    };

    typedef std::map<size_t,Chunk*> chunks_t;
    chunks_t m_chunks;

    size_t m_capacity;
    size_t m_size;
    size_t m_pos;

    explicit Impl(size_t hint);
    ~Impl();
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    void Ensure(size_t size);
};

MemoryStream::Impl::Chunk::Chunk(size_t l)
    : len(l)
{
    data = malloc(len);
}

MemoryStream::Impl::Chunk::~Chunk()
{
    free(data);
}

MemoryStream::Impl::Impl(size_t hint)
    : m_capacity(0), m_size(0), m_pos(0)
{
    if (hint)
	Ensure(hint);
}


MemoryStream::Impl::~Impl()
{
    for (chunks_t::iterator i = m_chunks.begin(); i != m_chunks.end(); ++i)
    {
	delete i->second;
	i->second = NULL;
    }
}

void MemoryStream::Impl::Ensure(size_t sz)
{
    if (sz > m_capacity)
    {
	const size_t ROUNDUP = 1024*1024;
	const size_t MASK = ROUNDUP-1;
	size_t add = ((sz - m_capacity) + MASK) & ~MASK;
	m_chunks.insert(std::make_pair(m_capacity, new Chunk(add)));
	m_capacity += add;
    }
}

unsigned MemoryStream::Impl::Write(const void *buf, size_t len, size_t *pwrote)
{
    Ensure(m_pos+len);

    if (!len)
    {
	*pwrote = 0;
	return 0;
    }
    
    chunks_t::iterator i = m_chunks.lower_bound(m_pos);

    if (i == m_chunks.end())
    {
//	TRACE << "Oh dear can't find m_pos " << m_pos << "\n";

//	for (chunks_t::iterator j = m_chunks.begin(); j != m_chunks.end(); ++j)
//	{
//	    TRACE << "  at " << j->first << " len " << j->second->len
//		  << " ends " << (j->first + j->second->len) << "\n";
//	}
	--i;
    }
//    TRACE << "i->first = " << i->first << " m_pos = " << m_pos << "\n";

    if (i->first > m_pos)
	--i;
    size_t chunkpos = m_pos - i->first;
    size_t nwrite = std::min(len, i->second->len - chunkpos);
    memcpy(((char*)i->second->data) + chunkpos, buf, nwrite);
    m_pos += nwrite;
    if (m_pos > m_size)
	m_size = m_pos;
    *pwrote = nwrite;
//    TRACE << "ms wrote " << nwrite << "/" << len << "\n";
    return 0;
}

unsigned MemoryStream::Impl::Read(void *buf, size_t len, size_t *pread)
{
    if (!len || m_pos >= m_size)
    {
	*pread = 0;
	return 0;
    }

    len = std::min(len, m_size-m_pos);
    chunks_t::iterator i = m_chunks.lower_bound(m_pos);
    if (i == m_chunks.end() || i->first > m_pos)
	--i;
//    TRACE << "Read: i->first = " << i->first << " m_pos = " << m_pos << "\n";
    size_t chunkpos = m_pos - i->first;
    size_t nread = std::min(len, i->second->len - chunkpos);
    memcpy(buf, ((const char*)i->second->data) + chunkpos, nread);
    m_pos += nread;
    *pread = nread;
    return 0;
}


MemoryStream::MemoryStream(size_t sizeHint)
    : m_impl(new Impl(sizeHint))
{
}

unsigned MemoryStream::Create(MemoryStreamPtr *pp, size_t sizeHint)
{
    *pp = MemoryStreamPtr(new MemoryStream(sizeHint));
    return 0;
}

unsigned MemoryStream::Write(const void *buf, size_t len, size_t *pwrote)
{
    return m_impl->Write(buf, len, pwrote);
}

unsigned MemoryStream::Read(void *buf, size_t len, size_t *pread)
{
    return m_impl->Read(buf, len, pread);
}

void MemoryStream::Seek(pos64 pos)
{
    if (pos > m_impl->m_size)
    {
	m_impl->m_pos = m_impl->m_size;
    }
    m_impl->m_pos = pos;
}

SeekableStream::pos64 MemoryStream::Tell()
{
    return m_impl->m_pos;
}

SeekableStream::pos64 MemoryStream::GetLength()
{
    return m_impl->m_size;
}

unsigned MemoryStream::SetLength(pos64 pos)
{
    m_impl->Ensure(pos);
    m_impl->m_size = pos;
    if (m_impl->m_pos > pos)
	m_impl->m_pos = pos;
    return 0;
}

MemoryStream::~MemoryStream()
{
//    TRACE << "~MemoryStream\n";
    delete m_impl;
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
