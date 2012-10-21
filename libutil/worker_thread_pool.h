#ifndef WORKER_THREAD_POOL_H
#define WORKER_THREAD_POOL_H 1

#include "attributes.h"
#include "task.h"
#include <list>
#include <deque>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <pthread.h>

namespace util {

namespace posix {
struct ThreadUser {  ThreadUser() {} };
}

namespace win32 {
#ifdef WIN32
struct ThreadUser
{
    ThreadUser() { pthread_win32_process_attach_np(); }
    ~ThreadUser() { pthread_win32_process_detach_np(); }
};
#endif
}

#ifdef WIN32
namespace threadpapi = ::util::win32;
#else
namespace threadpapi = ::util::posix;
#endif

using threadpapi::ThreadUser;

class WorkerThread;

class SimpleTaskQueue: public TaskQueue
{
    boost::mutex m_deque_mutex;
    boost::condition m_dequenotempty;
    typedef std::deque<TaskPtr> deque_t;
    deque_t m_deque;
    unsigned m_waiting;

public:
    SimpleTaskQueue();

    void PushTask(TaskPtr);
    TaskPtr PopTask(unsigned int timeout_sec);
    bool AnyWaiting();
    size_t Count();
};

class WorkerThreadPool: public TaskQueue
{
public:
    enum Priority {
	HIGH, ///< Real-time; use with CARE!
	NORMAL,
	LOW
    };

private:
    SimpleTaskQueue m_queue;
    Priority m_priority;
    unsigned int m_max_threads;
    boost::mutex m_mutex;
    std::list<WorkerThread*> m_threads;
    boost::condition m_threads_empty;

    void SuggestNewThread();

public:
    /** Create a thread pool with up to n threads. 
     *
     * If n=0, use one per CPU, plus one. Note that the default for a
     * pool is low priority; the default for an individual thread is
     * "normal". No threads are actually started until tasks are pushed.
     */
    explicit WorkerThreadPool(Priority, unsigned int n = 0);
    ~WorkerThreadPool();

    /** Forbid any more threads from starting, and join all existing ones.
     */
    void Shutdown();

    TaskPtr PopTaskOrQuit(WorkerThread*);

    // Being a TaskQueue
    void PushTask(TaskPtr);
    TaskPtr PopTask(unsigned int timeout_sec);
    bool AnyWaiting();
    size_t Count();
};

} // namespace util

#endif
