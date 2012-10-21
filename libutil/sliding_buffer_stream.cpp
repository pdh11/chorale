#include "sliding_buffer_stream.h"
#include "trace.h"
#include "errors.h"
#include <algorithm>

namespace util {

SlidingBufferStream::SlidingBufferStream(size_t n)
    : m_buffer(new unsigned char[n]),
      m_buffer_size(n),
      m_buffer_fill(0),
      m_buffer_offset(0),
      m_stream_offset(0),
      m_stream_length(0)
{
}

SlidingBufferStream::~SlidingBufferStream()
{
    delete[] m_buffer;
}

SeekableStream::pos64 SlidingBufferStream::GetLength()
{
    return m_stream_length;
}

unsigned SlidingBufferStream::SetLength(pos64)
{
    return EPERM;
}

unsigned SlidingBufferStream::WriteAt(const void *buffer, pos64 pos, 
				      size_t len, size_t *pwrote)
{
    boost::mutex::scoped_lock lock(m_mutex);

    if (len > m_buffer_size)
	len = m_buffer_size;

    if ((pos + len) > (m_stream_offset + m_buffer_size))
    {
	pos64 slide = (pos+len) - (m_stream_offset + m_buffer_size);
	m_stream_offset += slide;
	if (slide >= m_buffer_fill)
	{
	    m_buffer_offset = 0;
	    m_buffer_fill = 0;
	}
	else
	{
	    m_buffer_offset += (size_t)slide;
	    m_buffer_fill -= (size_t)slide;
	    if (m_buffer_offset >= m_buffer_size)
		m_buffer_offset -= m_buffer_size;
	}
    }

    assert((pos - m_stream_offset) < m_buffer_size);

    size_t write_offset = m_buffer_offset + (size_t)(pos - m_stream_offset);
    size_t lump;
    if (write_offset >= m_buffer_size)
    {
	write_offset -= m_buffer_size;
	lump = m_buffer_offset - write_offset;
    }
    else
    {
	lump = m_buffer_size - write_offset;
    }

    lump = std::min(len, lump);
    memcpy(m_buffer + write_offset, buffer, lump);
    *pwrote = lump;
    m_buffer_fill += lump;
    assert(m_buffer_fill <= m_buffer_size);

    if (pos+lump > m_stream_length)
	m_stream_length = pos+lump;

    return 0;
}

unsigned SlidingBufferStream::ReadAt(void *buffer, pos64 pos, size_t len, 
				     size_t *pread)
{
    boost::mutex::scoped_lock lock(m_mutex);

    if (pos < m_stream_offset)
    {
//	TRACE << "Forcing read@" << pos << " to " << m_stream_offset << "\n";
	pos = m_stream_offset;
    }

    if (pos + len > m_stream_offset + m_buffer_fill)
    {
	if (pos >= m_stream_offset + m_buffer_fill)
	{
	    *pread = 0;
	    return 0;
	}
	len = (size_t)(m_stream_offset + m_buffer_fill - pos);
    }

    size_t read_offset = m_buffer_offset + (size_t)(pos - m_stream_offset);
    size_t lump;
    if (read_offset >= m_buffer_size)
    {
	read_offset -= m_buffer_size;
	lump = m_buffer_offset - read_offset;
    }
    else
    {
	lump = m_buffer_size - read_offset;
    }

    lump = std::min(len, lump);
    memcpy(buffer, m_buffer + read_offset, lump);
    *pread = lump;
    return 0;
}


        /* EagerSlidingBufferStream */


EagerSlidingBufferStream::EagerSlidingBufferStream(size_t buffer_size,
						   StreamPtr upstream)
    : SlidingBufferStream(buffer_size),
      m_upstream(upstream),
      m_writepos(0)
{
}

unsigned EagerSlidingBufferStream::ReadAt(void *buffer, pos64 pos, size_t len,
					  size_t *pread)
{
    unsigned char temp[1024];

    for (;;)
    {
	boost::mutex::scoped_lock lock(m_mutex2);

	size_t nread = 0;
	unsigned int rc = m_upstream->Read(temp, sizeof(temp), &nread);

//	TRACE << "Upstream read returned " << rc << " nread=" << nread << "\n";

	if (rc && rc != EWOULDBLOCK)
	    return rc;

	if (rc || !nread)
	    break;

	rc = SlidingBufferStream::WriteAllAt(temp, m_writepos, nread);
	m_writepos += nread;
    }

    unsigned int rc = SlidingBufferStream::ReadAt(buffer, pos, len, pread);
    if (!rc)
    {
	if (*pread == 0)
	    return EWOULDBLOCK;
    }
    return rc;
}

util::PollHandle EagerSlidingBufferStream::GetReadHandle()
{
    util::PollHandle h = m_upstream->GetReadHandle();
//    TRACE << "ESBS handle=" << h << "\n";
    return h;
}

} // namespace util

#ifdef TEST

int main()
{
    util::SlidingBufferStream sbs(4096);

    // Fill the whole buffer
    for (unsigned int i=0; i<1024; ++i)
    {
	unsigned int j = (i * 37) ^ 0x6c503e21;
	size_t nwrote;
	unsigned int rc = sbs.WriteAt(&j, i*4, 4, &nwrote);
	assert(rc == 0);
	assert(nwrote == 4);
    }

    // Check it's still there
    for (unsigned int i=0; i<1024; ++i)
    {
	unsigned int j;
	size_t nread;
	unsigned int rc = sbs.ReadAt(&j, i*4, 4, &nread);
	assert(rc == 0);
	assert(nread == 4);
	assert(j == ((i * 37) ^ 0x6c503e21));
    }

    // Write a bit more
    for (unsigned int i=1024; i<1536; ++i)
    {
	unsigned int j = (i * 37) ^ 0x6c503e21;
	size_t nwrote;
	unsigned int rc = sbs.WriteAt(&j, i*4, 4, &nwrote);
	assert(rc == 0);
	assert(nwrote == 4);
    }

    // Check it's still there
    for (unsigned int i=512; i<1536; ++i)
    {
	unsigned int j;
	size_t nread;
	unsigned int rc = sbs.ReadAt(&j, i*4, 4, &nread);
	assert(rc == 0);
	assert(nread == 4);
	assert(j == ((i * 37) ^ 0x6c503e21));
    }

    // Check something earlier isn't
    unsigned int j;
    size_t nread;
    unsigned int rc = sbs.ReadAt(&j, 511*4, 4, &nread);
    assert(rc == 0); // not allowed to fail
    assert(nread == 4);
    assert(j == ((512*37) ^ 0x6c503e21)); // NB not 511

    return 0;
}

#endif
