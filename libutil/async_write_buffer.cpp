#include "async_write_buffer.h"
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "task.h"
#include "task_queue.h"
#include "mutex.h"
#include "trace.h"
#include "bind.h"
#include "counted_pointer.h"

namespace util {

class AsyncWriteBuffer::Impl
{
    class Task;
    typedef CountedPointer<Task> TaskPtr;

    friend class AsyncWriteBuffer;

    enum { BUFSIZE = 128*1024 };

    Stream *m_stream;
    TaskQueue *m_queue;

    util::Mutex m_mutex;
    util::Condition m_buffree;
    bool m_busy[2];

    /** Which buffer we're currently filling
     */
    int  m_filling;

    size_t m_bufpos; // How much of the currently-filling buffer is filled
    unsigned char *m_buf[2];
    CountedPointer<Task> m_writetask[2];
    uint64_t m_buffer_offset[2]; // Offset in the destination file of buffers
    uint64_t m_current_buffer_offset; // Offset in the destination file of curently-filling buffer
    unsigned int m_error;

    void SynchronousFlush();

public:
    Impl(Stream *stream, TaskQueue *queue);
    ~Impl();

    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len,
		     size_t *pwrote);
};

class AsyncWriteBuffer::Impl::Task: public util::Task
{
    AsyncWriteBuffer::Impl *m_parent;
    int m_which;
    Task(AsyncWriteBuffer::Impl *parent, int which);

public:
    static TaskPtr Create(AsyncWriteBuffer::Impl *parent, int which);

    // Being a Task
    unsigned int Run() override;
};

CountedPointer<AsyncWriteBuffer::Impl::Task>
AsyncWriteBuffer::Impl::Task::Create(AsyncWriteBuffer::Impl *parent,
				     int which)
{
    return TaskPtr(new AsyncWriteBuffer::Impl::Task(parent,which));
}

AsyncWriteBuffer::Impl::Task::Task(AsyncWriteBuffer::Impl *parent, int which)
    : m_parent(parent),
      m_which(which)
{}

unsigned int AsyncWriteBuffer::Impl::Task::Run()
{
    if (m_parent->m_error)
	return m_parent->m_error;

    unsigned int rc = m_parent->m_stream->WriteAllAt(m_parent->m_buf[m_which],
						     m_parent->m_buffer_offset[m_which],
						     AsyncWriteBuffer::Impl::BUFSIZE);
    if (rc)
    {
	TRACE << "warning, async write failed (" << rc << ")\n";
	m_parent->m_error = rc;
    }
//	else
//	    TRACE << "Async Wrote\n";

    util::Mutex::Lock lock(m_parent->m_mutex);
    m_parent->m_busy[m_which] = false;
    m_parent->m_buffree.NotifyOne();
    return 0;
}

AsyncWriteBuffer::Impl::Impl(Stream *stream, TaskQueue *queue)
    : m_stream(stream),
      m_queue(queue),
      m_filling(0),
      m_bufpos(0),
      m_current_buffer_offset(0),
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
	util::Mutex::Lock lock(m_mutex);

	while (m_busy[0] || m_busy[1])
	    m_buffree.Wait(lock, 60);
    }

    // Anything left, write synchronously
    if (m_bufpos)
    {
	unsigned int rc = m_stream->WriteAllAt(m_buf[m_filling],
					       m_current_buffer_offset,
					       m_bufpos);
	if (rc)
	{
	    TRACE << "warning, async write 2 failed (" << rc << ")\n";
	}
    }
    m_current_buffer_offset += m_bufpos;
    m_bufpos = 0;
}

AsyncWriteBuffer::Impl::~Impl()
{
    SynchronousFlush();
    delete[] m_buf[0];
    delete[] m_buf[1];
}

AsyncWriteBuffer::AsyncWriteBuffer(Stream *ptr, TaskQueue *queue)
    : m_impl(new Impl(ptr, queue))
{
}

AsyncWriteBuffer::~AsyncWriteBuffer()
{
    delete m_impl;
}

unsigned AsyncWriteBuffer::WriteAt(const void *buffer, uint64_t pos, size_t len, 
				   size_t *pwrote)
{
    if (m_impl->m_error)
	return m_impl->m_error;

    return m_impl->WriteAt(buffer, pos, len, pwrote);
}

unsigned AsyncWriteBuffer::Impl::WriteAt(const void *buffer, uint64_t pos,
					 size_t len, size_t *pwrote)
{
    if (pos != (m_current_buffer_offset + m_bufpos))
    {
	SynchronousFlush();
	m_current_buffer_offset = pos;
    }

    if (m_bufpos == 0)
    {
	util::Mutex::Lock lock(m_mutex);

	while (m_busy[0] && m_busy[1])
	    m_buffree.Wait(lock, 60);

	if (!m_busy[0])
	    m_filling = 0;
	else
	    m_filling = 1;
    }

    // We've got a buffer. Now write into it.

    size_t accept = std::min(len, BUFSIZE-m_bufpos);

    if (accept) {
        memcpy(m_buf[m_filling] + m_bufpos, buffer, accept);
    }
    *pwrote = accept;
    m_bufpos += accept;

    if (m_bufpos == BUFSIZE)
    {
	m_busy[m_filling] = true;
	m_buffer_offset[m_filling] = m_current_buffer_offset;
	m_current_buffer_offset += m_bufpos;
	m_queue->PushTask(Bind(m_writetask[m_filling]).To<&Task::Run>());
	m_bufpos = 0;
    }
    return 0;
}

/** Read, by contrast, goes straight to the underlying stream
 */
unsigned AsyncWriteBuffer::ReadAt(void *buffer, uint64_t pos, size_t len,
				  size_t *pread)
{
    m_impl->SynchronousFlush();
    return m_impl->m_stream->ReadAt(buffer, pos, len, pread);
}

unsigned AsyncWriteBuffer::SetLength(uint64_t len)
{
    m_impl->SynchronousFlush();
    unsigned rc = m_impl->m_stream->SetLength(len);
    if (rc)
	return rc;
    if (Tell() > len)
	Seek(len);
    return 0;
}

uint64_t AsyncWriteBuffer::GetLength()
{
    m_impl->SynchronousFlush();
    return m_impl->m_stream->GetLength();
}

} // namespace util

#ifdef TEST
# include "memory_stream.h"
# include "file_stream.h"
# include "stream_test.h"
# include "worker_thread_pool.h"

int main()
{
//    cpu_set_t cpus;
//    CPU_ZERO(&cpus);
//    CPU_SET(0, &cpus);
//    int rc0 = sched_setaffinity(0, sizeof(cpus), &cpus);
//    TRACE << "ssa returned " << rc0 << "\n";

    util::MemoryStream ms;

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);
    util::AsyncWriteBuffer asp(&ms, &wtp);
    TestSeekableStream(&asp);

    std::unique_ptr<util::Stream> fsp;

    unsigned int rc = util::OpenFileStream("test2.tmp", util::TEMP, &fsp);
    assert(rc == 0);

    util::AsyncWriteBuffer asp2(fsp.get(), &wtp);
    TestSeekableStream(&asp2);

    return 0;
}

#endif
