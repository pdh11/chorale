#include "config.h"
#include "trace.h"
#include "worker_thread_pool.h"
#include "cpus.h"
#include "counted_pointer.h"
#include "mutex.h"
#include "bind.h"
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
#if HAVE_WINDOWS_H
#include <windows.h>
#endif

namespace util {


        /* WorkerThread */


class WorkerThread
{
    WorkerThreadPool *m_owner;
    WorkerThreadPool::Priority m_priority;
    Thread m_thread;
    
public:
    WorkerThread(WorkerThreadPool *owner, WorkerThreadPool::Priority p) 
	: m_owner(owner),
	  m_priority(p),
	  m_thread(Bind<WorkerThread,&WorkerThread::Run>(this))
	{}

    ~WorkerThread()
    {
    }

    unsigned int Run();
};

unsigned int WorkerThread::Run()
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
	    if (errno != EPERM)
	    {
		TRACE << "Becoming real-time failed, errno " << errno << "\n";
	    }
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
	TaskCallback cb = m_owner->PopTaskOrQuit(this);
	if (!cb)
	{
//	    TRACE << "No task, quitting\n";
	    return 0;
	}

	cb();
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
    util::Mutex::Lock lock(m_mutex);
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

    util::Mutex::Lock lock(m_mutex);
//    TRACE << "wtp" << this << " Shutdown pushing\n";
    for (unsigned int i=0; i<m_threads.size(); ++i)
	m_queue.PushTask(TaskCallback());
//    TRACE << "Shutdown waiting\n";
    while (!m_threads.empty())
    {
	m_threads_empty.Wait(lock, 60);
    }
//    TRACE << "Shutdown done\n";
}

void WorkerThreadPool::PushTask(const TaskCallback& cb)
{
    if (m_max_threads == 0) // Shutting down
	return;

    if (!m_queue.AnyWaiting())
	SuggestNewThread();
    m_queue.PushTask(cb);
}

TaskCallback WorkerThreadPool::PopTaskOrQuit(WorkerThread *wt)
{
    TaskCallback cb = PopTask(10);
    if (!cb)
    {
	util::Mutex::Lock lock(m_mutex);
	m_threads.remove(wt);
	delete wt;
//	TRACE << "-Now " << m_threads.size() << " threads\n";
	if (m_threads.empty())
	    m_threads_empty.NotifyAll();
    }
    return cb;
}

TaskCallback WorkerThreadPool::PopTask(unsigned int timeout_sec)
{
    return m_queue.PopTask(timeout_sec);
}

bool WorkerThreadPool::AnyWaiting()
{
    if (m_queue.AnyWaiting())
	return true;

    util::Mutex::Lock lock(m_mutex);
    return m_threads.size() < m_max_threads;
}

size_t WorkerThreadPool::Count()
{
    util::Mutex::Lock lock(m_mutex);
    return m_queue.Count() + m_threads.size();
}


        /* SimpleTaskQueue */


SimpleTaskQueue::SimpleTaskQueue()
    : m_waiting(0)
{
}

void SimpleTaskQueue::PushTask(const TaskCallback& cb)
{
    util::Mutex::Lock lock(m_deque_mutex);
    m_deque.push_back(cb);
    m_dequenotempty.NotifyOne();
}

TaskCallback SimpleTaskQueue::PopTask(unsigned int timeout_sec)
{
    util::Mutex::Lock lock(m_deque_mutex);

    ++m_waiting;
    while (m_deque.empty())
    {
//	TRACE << m_waiting << " waiting\n";
	bool ret = m_dequenotempty.Wait(lock, timeout_sec);
//	TRACE << "waited " << ret << "\n";
	if (!ret)
	    break;
    }
    --m_waiting;
    if (m_deque.empty())
	return TaskCallback();

    TaskCallback result = m_deque.front();
    m_deque.pop_front();
    return result;
}

bool SimpleTaskQueue::AnyWaiting()
{
    util::Mutex::Lock lock(m_deque_mutex);
    return m_waiting > m_deque.size();
}

size_t SimpleTaskQueue::Count()
{
    util::Mutex::Lock lock(m_deque_mutex);
    return m_deque.size() - m_waiting;
}

} // namespace util

#ifdef TEST


static util::Mutex s_mx;
static unsigned int s_created = 0, s_destroyed = 0, s_run = 0;


class Snooze: public util::Task
{
public:
    Snooze();
    ~Snooze();

    unsigned int Run();
};

Snooze::Snooze()
{
    util::Mutex::Lock lock(s_mx);
    ++s_created;
}

Snooze::~Snooze()
{
    util::Mutex::Lock lock(s_mx);
    ++s_destroyed;
}

unsigned int Snooze::Run()
{
//    TRACE << "In run\n";
#ifdef WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
//    TRACE << "Trying to lock\n";
    util::Mutex::Lock lock(s_mx);
    ++s_run;
//    TRACE << "Completed\n";
    return 0;
}

typedef util::CountedPointer<Snooze> SnoozePtr;

void Test2(util::WorkerThreadPool::Priority p, unsigned int n)
{
    s_created = s_destroyed = s_run = 0;

    util::WorkerThreadPool wtp(p, n);

//    TRACE << "Created, pushing\n";

    for (unsigned int i=0; i<n*2; ++i)
    {
	wtp.PushTask(util::Bind<Snooze,&Snooze::Run>(SnoozePtr(new Snooze)));
    }

//    TRACE << "Shutting down\n";

    wtp.Shutdown();

//    TRACE << "Shutdown complete\n";

    assert(s_created == s_destroyed);

//    TRACE << "Test(" << n << ") done\n";
}

void Test(util::WorkerThreadPool::Priority p)
{
    Test2(p, 4);
    Test2(p, 1);
    Test2(p, 10);
    Test2(p, 100);
}

int main()
{
    Test(util::WorkerThreadPool::LOW);
    Test(util::WorkerThreadPool::NORMAL);
    Test(util::WorkerThreadPool::HIGH);
}

#endif
