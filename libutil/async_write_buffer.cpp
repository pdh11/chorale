#include "async_write_buffer.h"
#include <errno.h>
#include <string.h>
#include <boost/thread/condition.hpp>
#include "task.h"
#include "worker_thread_pool.h"
#include "memory_stream.h"
#include "file_stream.h"
#include "stream_test.h"
#include "trace.h"

namespace util {

class AsyncWriteBuffer::Impl
{
    class Task;

    friend class AsyncWriteBuffer;

    enum { BUFSIZE = 128*1024 };

    StreamPtr m_stream;
    TaskQueue *m_queue;

    boost::mutex m_mutex;
    boost::condition m_buffree;
    bool m_busy[2];
    int  m_whichbuffer;
    size_t m_bufpos;
    unsigned char *m_buf[2];
    TaskPtr m_writetask[2];

    /** Serialises writes, which (as we don't have pwrite) is essential to
     * avoid interleaving.
     *
     * @todo Theoretically this is still not safe, as the two tasks could
     *       get queued on different CPUs simultaneously, then run in the
     *       wrong order. Look into boost::asio::io_service::strand.
     */
    boost::mutex m_write_mutex;
    unsigned int m_error;

    void SynchronousFlush();

public:
    Impl(StreamPtr stream, TaskQueue *queue);
    ~Impl();

    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
};

class AsyncWriteBuffer::Impl::Task: public util::Task
{
    AsyncWriteBuffer::Impl *m_parent;
    int m_which;
    Task(AsyncWriteBuffer::Impl *parent, int which);

public:
    static TaskPtr Create(AsyncWriteBuffer::Impl *parent, int which);

    // Being a Task
    void Run();
};

TaskPtr AsyncWriteBuffer::Impl::Task::Create(AsyncWriteBuffer::Impl *parent,
					     int which)
{
    return TaskPtr(new AsyncWriteBuffer::Impl::Task(parent,which));
}

AsyncWriteBuffer::Impl::Task::Task(AsyncWriteBuffer::Impl *parent, int which)
    : m_parent(parent),
      m_which(which)
{}

void AsyncWriteBuffer::Impl::Task::Run()
{
    {
	boost::mutex::scoped_lock lock(m_parent->m_write_mutex);

	if (m_parent->m_error)
	    return;

	unsigned int rc = m_parent->m_stream->WriteAll(m_parent->m_buf[m_which],
						   AsyncWriteBuffer::Impl::BUFSIZE);
	if (rc)
	{
	    TRACE << "warning, async write failed (" << rc << ")\n";
	    m_parent->m_error = rc;
	}
//	else
//	    TRACE << "Async Wrote\n";
    }

    boost::mutex::scoped_lock lock(m_parent->m_mutex);
    m_parent->m_busy[m_which] = false;
    m_parent->m_buffree.notify_one();
}

AsyncWriteBuffer::Impl::Impl(StreamPtr stream, TaskQueue *queue)
    : m_stream(stream),
      m_queue(queue),
      m_whichbuffer(0),
      m_bufpos(0),
      m_error(0)
{
    m_busy[0] = false;
    m_busy[1] = false;
    m_buf[0] = new unsigned char[BUFSIZE];
    m_buf[1] = new unsigned char[BUFSIZE];
    m_writetask[0] = Task::Create(this, 0);
    m_writetask[1] = Task::Create(this, 1);
}

void AsyncWriteBuffer::Impl::SynchronousFlush()
{
    // Wait for tasks
    {
	boost::mutex::scoped_lock lock(m_mutex);

	while (m_busy[0] || m_busy[1])
	    m_buffree.wait(lock);
    }

    // Anything left, write synchronously
    if (m_bufpos)
    {
	unsigned int rc = m_stream->WriteAll(m_buf[m_whichbuffer], m_bufpos);
	if (rc)
	{
	    TRACE << "warning, async write 2 failed (" << rc << ")\n";
	}
    }
    m_bufpos = 0;
}

AsyncWriteBuffer::Impl::~Impl()
{
    SynchronousFlush();
    delete[] m_buf[0];
    delete[] m_buf[1];
}

AsyncWriteBuffer::AsyncWriteBuffer(StreamPtr ptr, TaskQueue *queue)
{
    m_impl = new Impl(ptr, queue);
}

AsyncWriteBuffer::~AsyncWriteBuffer()
{
    delete m_impl;
}

unsigned AsyncWriteBuffer::Create(StreamPtr backingstream,
				  TaskQueue *queue,
				  StreamPtr *result)
{
    *result = StreamPtr(new AsyncWriteBuffer(backingstream, queue));
    return 0;
}

unsigned AsyncWriteBuffer::Write(const void *buffer, size_t len, 
				 size_t *pwrote)
{
    if (m_impl->m_error)
	return m_impl->m_error;

    return m_impl->Write(buffer, len, pwrote);
}

unsigned AsyncWriteBuffer::Impl::Write(const void *buffer, size_t len, 
				 size_t *pwrote)
{
    if (m_bufpos == 0)
    {
	boost::mutex::scoped_lock lock(m_mutex);

	while (m_busy[0] && m_busy[1])
	    m_buffree.wait(lock);

	if (!m_busy[0])
	    m_whichbuffer = 0;
	else
	    m_whichbuffer = 1;
    }

    // We've got a buffer. Now write into it.

    size_t accept = std::min(len, BUFSIZE-m_bufpos);
	
    memcpy(m_buf[m_whichbuffer] + m_bufpos, buffer, accept);
    *pwrote = accept;
    m_bufpos += accept;

    if (m_bufpos == BUFSIZE)
    {
	m_busy[m_whichbuffer] = true;
	m_queue->PushTask(m_writetask[m_whichbuffer]);
	m_bufpos = 0;
    }
    return 0;
}

/** Read, by contrast, goes straight to the underlying stream
 */
unsigned AsyncWriteBuffer::Read(void *buffer, size_t len, size_t *pread)
{
    m_impl->SynchronousFlush();
    return m_impl->m_stream->Read(buffer, len, pread);
}

} // namespace util

#ifdef TEST

int main()
{
    util::MemoryStreamPtr msp;

//    cpu_set_t cpus;
//    CPU_ZERO(&cpus);
//    CPU_SET(0, &cpus);
//    int rc0 = sched_setaffinity(0, sizeof(cpus), &cpus);
//    TRACE << "ssa returned " << rc0 << "\n";

    unsigned int rc = util::MemoryStream::Create(&msp);
    assert(rc == 0);

    util::WorkerThreadPool wtp(1);

    util::StreamPtr asp;
    rc = util::AsyncWriteBuffer::Create(msp, wtp.GetTaskQueue(), &asp);

//    TestSeekableStream(asp);

    util::FileStreamPtr fsp;

    rc = util::FileStream::CreateTemporary("test2.tmp", &fsp);
    assert(rc == 0);

    util::StreamPtr asp2;
    rc = util::AsyncWriteBuffer::Create(fsp, wtp.GetTaskQueue(), &asp2);

//  TestSeekableStream(asp2);

    return 0;
}

#endif
