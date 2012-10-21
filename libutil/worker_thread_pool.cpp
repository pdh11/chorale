#include "trace.h"
#include "worker_thread_pool.h"
#include "worker_thread.h"
#include "cpus.h"

namespace util {

WorkerThreadPool::WorkerThreadPool(unsigned int n)
{
    if (!n)
    {
	n = util::CountCPUs();

	if (n < 2)
	    n = 2;
    }

    m_queue.SetNThreads(n);

    m_threads.resize(n);

    for (unsigned int i=0; i<n; ++i)
	m_threads[i] = new WorkerThread(&m_queue, WorkerThread::LOW);
}

WorkerThreadPool::~WorkerThreadPool()
{
    for (unsigned int i=0; i<m_threads.size(); ++i)
	m_queue.PushTask(TaskPtr());
    for (unsigned int i=0; i<m_threads.size(); ++i)
	delete m_threads[i];
}

}; // namespace util
