#include "config.h"
#include "trace.h"
#include "worker_thread_pool.h"
#include "cpus.h"
#include <boost/thread/xtime.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_SCHED_H
#include <sched.h>
#endif

namespace util {


        /* WorkerThread */


class WorkerThread
{
    WorkerThreadPool *m_owner;
    WorkerThreadPool::Priority m_priority;
    boost::thread m_thread;
    
public:
    WorkerThread(WorkerThreadPool *owner, WorkerThreadPool::Priority p) 
	: m_owner(owner),
	  m_priority(p),
	  m_thread(boost::bind(&WorkerThread::Run, this))
	{}

    ~WorkerThread()
    {
    }

    void Run();
};

void WorkerThread::Run()
{
    if (m_priority == WorkerThreadPool::LOW)
    {
#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
#elif HAVE_SETPRIORITY
	/* Both in linuxthreads and in NPTL, this affects only this thread, not
	 * the whole process (fortunately).
	 */
	setpriority(PRIO_PROCESS, 0, 15);
#endif
    }
    else if (m_priority == WorkerThreadPool::HIGH)
    {
#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#elif HAVE_SCHED_SETSCHEDULER

	struct sched_param param;
	param.sched_priority = 1;
	int rc = sched_setscheduler(0, SCHED_RR, &param);
	if (rc<0)
	{
	    TRACE << "Becoming real-time failed, errno " << errno << "\n";
# if HAVE_SETPRIORITY
	    setpriority(PRIO_PROCESS, 0, -15);
# endif
	}

#elif HAVE_SETPRIORITY
	/* Both in linuxthreads and in NPTL, this affects only this thread, not
	 * the whole process (fortunately).
	 */
	setpriority(PRIO_PROCESS, 0, -15);
#endif
    }

    for (;;)
    {
	TaskPtr p = m_owner->PopTaskOrQuit(this);
	if (!p)
	{
//	    TRACE << "No task, quitting\n";
	    return;
	}

	p->Run();
    }
}


        /* WorkerThreadPool */


WorkerThreadPool::WorkerThreadPool(Priority p, unsigned int n)
    : m_priority(p),
      m_max_threads(n ? n : (util::CountCPUs() + 1))
{
}

void WorkerThreadPool::SuggestNewThread()
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_threads.size() >= m_max_threads)
    {
//	TRACE << "Max " << m_max_threads << " threads, holding\n";
	return;
    }
    WorkerThread *thread = new WorkerThread(this, m_priority);
    m_threads.push_back(thread);
//    TRACE << "+Now " << m_threads.size() << " threads\n";
}

WorkerThreadPool::~WorkerThreadPool()
{
    Shutdown();
}

void WorkerThreadPool::Shutdown()
{
    m_max_threads = 0; // Stop new ones being created

//    TRACE << "wtp" << this << " Shutdown pushing\n";
    for (unsigned int i=0; i<m_threads.size(); ++i)
	m_queue.PushTask(TaskPtr());
//    TRACE << "Shutdown waiting\n";
    boost::mutex::scoped_lock lock(m_mutex);
    while (!m_threads.empty())
    {
	m_threads_empty.wait(lock);
    }
//    TRACE << "Shutdown done\n";
}

void WorkerThreadPool::PushTask(TaskPtr t)
{
    if (m_max_threads == 0) // Shutting down
	return;

    if (!m_queue.AnyWaiting())
	SuggestNewThread();
    m_queue.PushTask(t);
}

TaskPtr WorkerThreadPool::PopTaskOrQuit(WorkerThread *wt)
{
    TaskPtr p = PopTask(10);
    if (!p)
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_threads.remove(wt);
	delete wt;
//	TRACE << "-Now " << m_threads.size() << " threads\n";
	if (m_threads.empty())
	    m_threads_empty.notify_all();
    }
    return p;
}

TaskPtr WorkerThreadPool::PopTask(unsigned int timeout_sec)
{
    return m_queue.PopTask(timeout_sec);
}

bool WorkerThreadPool::AnyWaiting()
{
    if (m_queue.AnyWaiting())
	return true;

    boost::mutex::scoped_lock lock(m_mutex);
    return m_threads.size() < m_max_threads;
}

size_t WorkerThreadPool::Count()
{
    boost::mutex::scoped_lock lock(m_mutex);
    return m_queue.Count() + m_threads.size();
}


        /* SimpleTaskQueue */


SimpleTaskQueue::SimpleTaskQueue()
    : m_waiting(0)
{
}

void SimpleTaskQueue::PushTask(TaskPtr t)
{
    boost::mutex::scoped_lock lock(m_deque_mutex);
    m_deque.push_back(t);
    m_dequenotempty.notify_one();
}

TaskPtr SimpleTaskQueue::PopTask(unsigned int timeout_sec)
{
    boost::mutex::scoped_lock lock(m_deque_mutex);

    ++m_waiting;
    while (m_deque.empty())
    {
//	TRACE << m_waiting << " waiting\n";
	boost::xtime wakeup;
	boost::xtime_get(&wakeup, boost::TIME_UTC);
	wakeup.sec += timeout_sec;
	bool ret = m_dequenotempty.timed_wait(lock, wakeup);
//	TRACE << "waited " << ret << "\n";
	if (!ret)
	    break;
    }
    --m_waiting;
    if (m_deque.empty())
	return TaskPtr();

    TaskPtr result = m_deque.front();
    m_deque.pop_front();
    return result;
}

bool SimpleTaskQueue::AnyWaiting()
{
    boost::mutex::scoped_lock lock(m_deque_mutex);
    return m_waiting > m_deque.size();
}

size_t SimpleTaskQueue::Count()
{
    boost::mutex::scoped_lock lock(m_deque_mutex);
    return m_deque.size() - m_waiting;
}

} // namespace util
