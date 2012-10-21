#include "multi_stream.h"
#include "bind.h"
#include "task.h"
#include "trace.h"
#include "mutex.h"
#include "errors.h"
#include "counted_pointer.h"
#include <string.h>
#include <algorithm>

namespace util {

class MultiStream::Impl
{
    class Output;

    std::auto_ptr<Stream> m_stm;
    uint64_t m_size;
    uint64_t m_inputpos;
    unsigned m_noutputs;

    enum { MAX_OP = 4 };

    uint64_t m_outputposes[MAX_OP];
    util::TaskCallback m_callbacks[MAX_OP];

    util::Mutex m_mutex;

    unsigned OutputRead(void *buffer, size_t len, size_t *pread, unsigned ix);
    unsigned OutputWait(const util::TaskCallback& callback, unsigned ix);

public:
    Impl(std::auto_ptr<Stream> stm, uint64_t size);
    ~Impl();

    unsigned CreateOutput(MultiStream*, std::auto_ptr<Stream>*);
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
    unsigned GetStreamFlags() const { return READABLE|WAITABLE; }
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void*, size_t, size_t*) { return EACCES; }
    unsigned Wait(const util::TaskCallback&);
};


        /* MultiStream::Impl::Output */


unsigned MultiStream::Impl::Output::Read(void *buffer, size_t len,
					 size_t *pread)
{
    return m_parent->m_impl->OutputRead(buffer, len, pread, m_index);
}

unsigned MultiStream::Impl::Output::Wait(const util::TaskCallback& callback)
{
    return m_parent->m_impl->OutputWait(callback, m_index);
}


        /* MultiStream::Impl */


MultiStream::Impl::Impl(std::auto_ptr<Stream> stm, uint64_t size)
    : m_stm(stm), m_size(size), m_inputpos(0), m_noutputs(0)
{
    memset(m_outputposes, 0, MAX_OP*sizeof(uint64_t));
}

MultiStream::Impl::~Impl()
{
}

unsigned MultiStream::Impl::CreateOutput(MultiStream *parent, 
					 std::auto_ptr<Stream> *pp)
{
    if (m_inputpos)
	return EINVAL;
    pp->reset(new Output(parent, m_noutputs++));
    return 0;
}

unsigned MultiStream::Impl::OutputRead(void *buffer, size_t len,
				       size_t *pread, unsigned ix)
{
    if (m_outputposes[ix] == m_size)
    {
	TRACE << "OutputRead at EOF (" << m_size << ")\n";
	*pread = 0;
	return 0;
    }

    if (m_outputposes[ix] == m_size || len == 0)
    {
	*pread = 0;
	return 0;
    }

    uint64_t offset;
    {
	util::Mutex::Lock lock(m_mutex);

	assert(m_inputpos >= m_outputposes[ix]);
	offset = m_inputpos - m_outputposes[ix];
//	TRACE << "inputpos=" << m_inputpos << " outputpos=" << m_outputposes[ix]
//	      << " offset=" << offset << "\n";
    }

    if (!offset)
    {
//	TRACE << "MultiStream returns EWOULDBLOCK to #" << ix << "\n";
	return EWOULDBLOCK;
    }

    size_t nread = (size_t)std::min(offset, (uint64_t)len);
    assert(nread > 0);

    unsigned rc = m_stm->ReadAt(buffer, m_outputposes[ix], nread, &nread);
    if (rc)
    {
	TRACE << "OutputRead failed on backing stream: " << rc << "\n";
	return rc;
    }
    m_outputposes[ix] += nread;
    *pread = nread;
    return 0;
}

unsigned MultiStream::Impl::OutputWait(const util::TaskCallback& callback,
				       unsigned ix)
{
    bool doit = false;
    {
	util::Mutex::Lock lock(m_mutex);

	/* Has more data been written in-between Write() returning EWOULDBLOCK,
	 * and Wait() being called?
	 */
	if (m_outputposes[ix] != m_inputpos)
	    doit = true;
	else
	{
	    /* No, it's a real wait, store it
	     */
//	TRACE << "MultiStream stores callback for #" << ix << "\n";
	    m_callbacks[ix] = callback;
	}
    }

    if (doit) // With mutex not locked
	callback();
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

    {
    	unsigned rc = m_stm->WriteAt(buffer, m_inputpos, len, pwrote);
	if (rc)
	    return rc;

	util::Mutex::Lock lock(m_mutex);
	m_inputpos += *pwrote;
    }

    for (unsigned int i=0; i<m_noutputs; ++i)
    {
	util::TaskCallback cb;
	std::swap(cb, m_callbacks[i]);
	if (cb.IsValid())
	{
//	    TRACE << "MultiStream makes callback for #" << i << "\n";
	    cb();
	}
    }
    return 0;
}


        /* MultiStream */


MultiStreamPtr MultiStream::Create(std::auto_ptr<Stream> stm, uint64_t size)
{
    return MultiStreamPtr(new MultiStream(stm, size));
}

MultiStream::MultiStream(std::auto_ptr<Stream> stm, uint64_t size)
    : m_impl(new Impl(stm, size))
{
}

MultiStream::~MultiStream()
{
    TRACE << "~MultiStream\n";
    delete m_impl;
}

unsigned MultiStream::CreateOutput(std::auto_ptr<Stream> *pp)
{
    return m_impl->CreateOutput(this, pp);
}

unsigned MultiStream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    return m_impl->Write(buffer, len, pwrote);
}

} // namespace util
