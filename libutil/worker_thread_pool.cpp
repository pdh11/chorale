#include "config.h"
#include "trace.h"
#include "worker_thread_pool.h"
#include "cpus.h"
#include "counted_pointer.h"
#include "bind.h"
#include "task.h"
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
    std::thread m_thread;
    
public:
    WorkerThread(WorkerThreadPool *owner, WorkerThreadPool::Priority p) 
	: m_owner(owner),
	  m_priority(p),
	  m_thread(Bind(this).To<&WorkerThread::Run>())
	{}

    ~WorkerThread()
    {
        m_thread.detach();
    }

    unsigned int Run();
};

unsigned int WorkerThread::Run()
{
    if (m_priority == WorkerThreadPool::LOW)
    {
#if HAVE_SETPRIORITY
	/* Both in linuxthreads and in NPTL, this affects only this thread, not
	 * the whole process (fortunately).
	 */
	setpriority(PRIO_PROCESS, 0, 15);
#endif
    }
    else if (m_priority == WorkerThreadPool::HIGH)
    {
#if HAVE_SCHED_SETSCHEDULER

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
	if (!cb.IsValid())
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
    std::lock_guard<std::mutex> lock(m_mutex);
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

    std::unique_lock<std::mutex> lock(m_mutex);
//    TRACE << "wtp" << this << " Shutdown pushing\n";
    for (unsigned int i=0; i<m_threads.size(); ++i)
	m_queue.PushTask(TaskCallback());
//    TRACE << "Shutdown waiting\n";
    while (!m_threads.empty())
    {
	m_threads_empty.wait_for(lock, std::chrono::seconds(60));
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

void WorkerThreadPool::PushTaskFront(const TaskCallback& cb)
{
    if (m_max_threads == 0) // Shutting down
	return;

    if (!m_queue.AnyWaiting())
	SuggestNewThread();
    m_queue.PushTaskFront(cb);
}

TaskCallback WorkerThreadPool::PopTaskOrQuit(WorkerThread *wt)
{
    TaskCallback cb = PopTask(10);
    if (!cb.IsValid())
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threads.remove(wt);
	delete wt;
//	TRACE << "-Now " << m_threads.size() << " threads\n";
        if (m_threads.empty())
            m_threads_empty.notify_all();
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

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_threads.size() < m_max_threads;
}

size_t WorkerThreadPool::Count()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.Count() + m_threads.size();
}


        /* SimpleTaskQueue */


SimpleTaskQueue::SimpleTaskQueue()
    : m_waiting(0)
{
}

void SimpleTaskQueue::PushTask(const TaskCallback& cb)
{
    std::lock_guard<std::mutex> lock(m_deque_mutex);
    m_deque.push_back(cb);
    m_dequenotempty.notify_one();
}

void SimpleTaskQueue::PushTaskFront(const TaskCallback& cb)
{
    std::lock_guard<std::mutex> lock(m_deque_mutex);
    m_deque.push_front(cb);
    m_dequenotempty.notify_one();
}

TaskCallback SimpleTaskQueue::PopTask(unsigned int timeout_sec)
{
    std::unique_lock<std::mutex> lock(m_deque_mutex);

    ++m_waiting;
    while (m_deque.empty())
    {
//	TRACE << m_waiting << " waiting\n";
	if (m_dequenotempty.wait_for(lock,
                                     std::chrono::seconds(timeout_sec))
            == std::cv_status::timeout) {
//	TRACE << "waited " << ret << "\n";
	    break;
        }
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
    std::lock_guard<std::mutex> lock(m_deque_mutex);
    return m_waiting > m_deque.size();
}

size_t SimpleTaskQueue::Count()
{
    std::lock_guard<std::mutex> lock(m_deque_mutex);
    return m_deque.size() - m_waiting;
}

} // namespace util

#ifdef TEST


static std::mutex s_mx;
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
    std::lock_guard<std::mutex> lock(s_mx);
    ++s_created;
}

Snooze::~Snooze()
{
    std::lock_guard<std::mutex> lock(s_mx);
    ++s_destroyed;
}

unsigned int Snooze::Run()
{
//    TRACE << "In run\n";
    sleep(1);
//    TRACE << "Trying to lock\n";
    std::lock_guard<std::mutex> lock(s_mx);
    ++s_run;
//    TRACE << "Completed\n";
    return 0;
}

typedef util::CountedPointer<Snooze> SnoozePtr;

static void Test2(util::WorkerThreadPool::Priority p, unsigned int n)
{
    s_created = s_destroyed = s_run = 0;

    util::WorkerThreadPool wtp(p, n);

//    TRACE << "Created, pushing\n";

    for (unsigned int i=0; i<n*2; ++i)
    {
	wtp.PushTask(util::Bind(SnoozePtr(new Snooze)).To<&Snooze::Run>());
    }

//    TRACE << "Shutting down\n";

    wtp.Shutdown();

//    TRACE << "Shutdown complete\n";

    assert(s_created == s_destroyed);

//    TRACE << "Test(" << n << ") done\n";
}

static void Test(util::WorkerThreadPool::Priority p)
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
