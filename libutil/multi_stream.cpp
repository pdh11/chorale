#include "multi_stream.h"
#include "trace.h"
#include <errno.h>
#include <boost/thread/condition.hpp>

namespace util {

class MultiStream::Impl
{
    class Output;

    SeekableStreamPtr m_stm;
    size_t m_size;
    size_t m_inputpos;
    unsigned m_noutputs;

    enum { MAX_OP = 4 };

    size_t m_outputposes[MAX_OP];

    boost::mutex m_mutex;
    boost::condition m_moredata;

    unsigned OutputRead(void *buffer, size_t len, size_t *pread, unsigned ix);

public:
    Impl(SeekableStreamPtr stm, size_t size);
    ~Impl();

    unsigned CreateOutput(MultiStream*, StreamPtr*);
    unsigned Write(const void*, size_t, size_t*);
};

class MultiStream::Impl::Output: public Stream
{
    MultiStreamPtr m_parent;
    unsigned m_index;

public:
    Output(MultiStream *parent, unsigned index)
	: m_parent(parent), m_index(index) {}

    // Being a Stream (this is the output side, so no writing)
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void*, size_t, size_t*) { return EACCES; }
};


        /* MultiStream::Impl::Output */


unsigned MultiStream::Impl::Output::Read(void *buffer, size_t len,
					 size_t *pread)
{
    return m_parent->m_impl->OutputRead(buffer, len, pread, m_index);
}


        /* MultiStream::Impl */


MultiStream::Impl::Impl(SeekableStreamPtr stm, size_t size)
    : m_stm(stm), m_size(size), m_inputpos(0), m_noutputs(0)
{
    memset(m_outputposes, 0, MAX_OP*sizeof(size_t));
}

MultiStream::Impl::~Impl()
{
}

unsigned MultiStream::Impl::CreateOutput(MultiStream *parent, StreamPtr *pp)
{
    if (m_inputpos)
	return EINVAL;
    *pp = StreamPtr(new Output(parent, m_noutputs++));
    return 0;
}


unsigned MultiStream::Impl::OutputRead(void *buffer, size_t len,
				       size_t *pread, unsigned ix)
{
    if (m_outputposes[ix] == m_size || len == 0)
    {
	*pread = 0;
	return 0;
    }

    boost::mutex::scoped_lock lock(m_mutex);

    while (m_outputposes[ix] == m_inputpos)
    {
	m_moredata.wait(lock);
    }

    size_t nread = m_inputpos - m_outputposes[ix];
    nread = std::min(nread, len);
    m_stm->Seek(m_outputposes[ix]);
    unsigned rc = m_stm->Read(buffer, nread, &nread);
    if (rc)
    {
	TRACE << "OutputRead failed on backing stream: " << rc << "\n";
	return rc;
    }
    m_outputposes[ix] += nread;
    *pread = nread;
    return 0;
}

unsigned MultiStream::Impl::Write(const void *buffer, size_t len,
				  size_t *pwrote)
{
    if (!len)
    {
	*pwrote = 0;
	return 0;
    }

    boost::mutex::scoped_lock lock(m_mutex);
    
    m_stm->Seek(m_inputpos);
    unsigned rc = m_stm->Write(buffer, len, pwrote);
    if (rc)
	return rc;
    m_inputpos += *pwrote;
    m_moredata.notify_all();
    return 0;
}


        /* MultiStream */


unsigned MultiStream::Create(SeekableStreamPtr back, size_t size, 
			     MultiStreamPtr *result)
{
    *result = MultiStreamPtr(new MultiStream(back, size));
    return 0;
}

MultiStream::MultiStream(SeekableStreamPtr stm, size_t size)
    : m_impl(new Impl(stm, size))
{
}

MultiStream::~MultiStream()
{
    TRACE << "~MultiStream\n";
    delete m_impl;
}

unsigned MultiStream::CreateOutput(StreamPtr *pp)
{
    return m_impl->CreateOutput(this, pp);
}

unsigned MultiStream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    return m_impl->Write(buffer, len, pwrote);
}

} // namespace util
