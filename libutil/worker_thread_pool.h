#ifndef WORKER_THREAD_POOL_H
#define WORKER_THREAD_POOL_H 1

#include "attributes.h"
#include "task.h"
#include "mutex.h"
#include <list>
#include <deque>

namespace util {

class WorkerThread;

class SimpleTaskQueue: public TaskQueue
{
    util::Mutex m_deque_mutex;
    util::Condition m_dequenotempty;
    typedef std::deque<TaskCallback> deque_t;
    deque_t m_deque;
    unsigned m_waiting;

public:
    SimpleTaskQueue();

    void PushTask(const TaskCallback&);
    void PushTaskFront(const TaskCallback&);
    TaskCallback PopTask(unsigned int timeout_sec);
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
    util::Mutex m_mutex;
    std::list<WorkerThread*> m_threads;
    util::Condition m_threads_empty;

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

    /** Obtain the next task, or, if there are none, a null task that means
     * to quit.
     */
    TaskCallback PopTaskOrQuit(WorkerThread*);

    // Being a TaskQueue
    void PushTask(const TaskCallback&);
    void PushTaskFront(const TaskCallback&);
    TaskCallback PopTask(unsigned int timeout_sec);
    bool AnyWaiting();
    size_t Count();
};

} // namespace util

#endif
