#ifndef WORKER_THREAD_POOL_H
#define WORKER_THREAD_POOL_H 1

#include "task.h"
#include "mutex.h"
#include <list>
#include <deque>

namespace util {

class WorkerThread;

class SimpleTaskQueue final: public TaskQueue
{
    util::Mutex m_deque_mutex;
    util::Condition m_dequenotempty;
    typedef std::deque<TaskCallback> deque_t;
    deque_t m_deque;
    unsigned m_waiting;

public:
    SimpleTaskQueue();
    TaskCallback PopTask(unsigned int timeout_sec);

    // Being a TaskQueue
    void PushTask(const TaskCallback&) override;
    void PushTaskFront(const TaskCallback&) override;
    bool AnyWaiting() override;
    size_t Count() override;
};

class WorkerThreadPool final: public TaskQueue
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
    TaskCallback PopTask(unsigned int timeout_sec);

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
    void PushTask(const TaskCallback&) override;
    void PushTaskFront(const TaskCallback&) override;
    bool AnyWaiting() override;
    size_t Count() override;
};

} // namespace util

#endif
