#ifndef POLLABLE_TASK_H
#define POLLABLE_TASK_H 1

#include "task.h"
#include "locking.h"
#include <set>
#include <time.h>

namespace util {

class Pollable;
class PollerInterface;
class WorkerThreadPool;

/** Manages tasks (in the task.h sense) which wish to sleep and get re-Run.
 */
class TaskPoller: private util::PerObjectRecursiveLocking
{
    PollerInterface *m_poller;
    WorkerThreadPool *m_thread_pool;

    class WaitingTask;
    std::set<WaitingTask*> m_waiting_tasks; // protected by mutex

    bool m_exiting;

    friend class WaitingTask;

public:
    TaskPoller(PollerInterface *poller,
	       WorkerThreadPool *thread_pool);
    ~TaskPoller();

    void Wait(TaskPtr ptr, Pollable*, unsigned int direction);
    void Wait(TaskPtr ptr, time_t timer);
};

class PollableTask: public Task
{
    TaskPoller *m_poller;

protected:
    void WaitForReadable(Pollable*);
    void WaitForWritable(Pollable*);

    void WaitForTimer(time_t);

    /** Because so many Pollables are CountedPointers, provide an overload.
     *
     * Only compiles if T.get() is a subclass of Pollable.
     */
    template <class T>
    void WaitForReadable(boost::intrusive_ptr<T>& ptr)
    {
	WaitForReadable(ptr.get());
    }

    template <class T>
    void WaitForWritable(boost::intrusive_ptr<T>& ptr)
    {
	WaitForWritable(ptr.get());
    }

public:
    PollableTask(TaskPoller *poller) : m_poller(poller) {}
};

} // namespace util

#endif
