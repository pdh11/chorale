#include "memory_stream.h"
#include "trace.h"
#include "mutex.h"
#include <map>
#include <string.h>
#include <stdlib.h>

namespace util {

class MemoryStream::Impl
{
    Mutex m_mutex;
public:
    struct Chunk
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

    explicit Impl(size_t hint);
    ~Impl();
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
		    size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len,
		     size_t *pwrote);
    void Ensure(size_t size);
};

MemoryStream::Impl::Chunk::Chunk(size_t l)
    : len(l), 
      data(NULL)
{
    data = malloc(len);
    memset(data, '\0', len);
}

MemoryStream::Impl::Chunk::~Chunk()
{
    free(data);
}

MemoryStream::Impl::Impl(size_t hint)
    : m_capacity(0), m_size(0)
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
	Mutex::Lock lock(m_mutex);

	const size_t ROUNDUP = 1024*1024;
	const size_t MASK = ROUNDUP-1;
	size_t add = ((sz - m_capacity) + MASK) & ~MASK;
	m_chunks.insert(std::make_pair(m_capacity, new Chunk(add)));
	m_capacity += add;
    }
}

unsigned MemoryStream::Impl::WriteAt(const void *buf, 
				     uint64_t pos, size_t len,
				     size_t *pwrote)
{
    Ensure((size_t)pos+len);

    if (!len)
    {
	*pwrote = 0;
	return 0;
    }
    
    chunks_t::iterator i;
    {
	Mutex::Lock lock(m_mutex);
	i = m_chunks.lower_bound((size_t)pos);

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
	
	if (i->first > pos)
	    --i;
    }
    size_t chunkpos = (size_t)pos - i->first;
    size_t nwrite = std::min(len, i->second->len - chunkpos);
    memcpy(((char*)i->second->data) + chunkpos, buf, nwrite);
    pos += nwrite;
    if (pos > m_size)
	m_size = (size_t)pos;
    *pwrote = nwrite;
//    TRACE << "ms wrote " << nwrite << "/" << len << "\n";
    return 0;
}

unsigned MemoryStream::Impl::ReadAt(void *buf, uint64_t pos,
				    size_t len, size_t *pread)
{
    if (!len || pos >= m_size)
    {
	*pread = 0;
	return 0;
    }

    len = std::min(len, m_size - (size_t)pos);
    chunks_t::iterator i;
    {
	Mutex::Lock lock(m_mutex);
	i = m_chunks.lower_bound((size_t)pos);
	if (i == m_chunks.end() || i->first > pos)
	    --i;
    }

//    TRACE << "Read: i->first = " << i->first << " pos = " << pos << "\n";
    size_t chunkpos = (size_t)pos - i->first;
    size_t nread = std::min(len, i->second->len - chunkpos);
    memcpy(buf, ((const char*)i->second->data) + chunkpos, nread);
    *pread = nread;
    return 0;
}


MemoryStream::MemoryStream(size_t sizeHint)
    : m_impl(new Impl(sizeHint))
{
}

unsigned MemoryStream::WriteAt(const void *buf, uint64_t pos, size_t len, 
			       size_t *pwrote)
{
    return m_impl->WriteAt(buf, pos, len, pwrote);
}

unsigned MemoryStream::ReadAt(void *buf, uint64_t pos, size_t len, size_t *pread)
{
    return m_impl->ReadAt(buf, pos, len, pread);
}

uint64_t MemoryStream::GetLength()
{
    return m_impl->m_size;
}

unsigned MemoryStream::SetLength(uint64_t pos)
{
    m_impl->Ensure((size_t)pos);
    m_impl->m_size = (size_t)pos;
    if (Tell() > pos)
	Seek(pos);
    return 0;
}

MemoryStream::~MemoryStream()
{
//    TRACE << "~MemoryStream\n";
    delete m_impl;
}

} // namespace util

#ifdef TEST
# include "stream_test.h"

int main()
{
    util::MemoryStream ms;
    TestSeekableStream(&ms);
    return 0;
}

#endif
