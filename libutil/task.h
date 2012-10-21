#ifndef TASK_H
#define TASK_H

#include <string>
#include <deque>
#include <boost/thread/condition.hpp>
#include "counted_object.h"

namespace util {

class Task;

class TaskObserver
{
public:
    virtual ~TaskObserver() {}

    virtual void OnProgress(const Task*, unsigned num, unsigned denom) = 0;
    virtual void OnCancelled(const Task*) {} // NYI
    virtual void OnError(const Task*, unsigned int /*error*/) {}
};

/** A Task is a unit of work suitable for handing off to a background
 * thread. It can send progress (but the progress callback is made on
 * the background thread). To hand off a Task to a WorkerThread, push
 * it on the WorkerThread's TaskQueue.
 */
class Task: public util::CountedObject<>
{
    boost::mutex m_observer_mutex;
    TaskObserver *m_observer;
    bool m_done;

protected:
    void FireProgress(unsigned num, unsigned denom);
    void FireError(unsigned int error);

public:
    Task();
    virtual ~Task() {}
    virtual void Run() = 0;
    virtual void Cancel() {} // NYI
    void SetObserver(TaskObserver*);
};

typedef boost::intrusive_ptr<Task> TaskPtr;


/** A queue of tasks waiting to be executed on one (or more)
 * background threads.
 */
class TaskQueue
{
    boost::mutex m_deque_mutex;
    boost::condition m_dequenotempty;
    typedef std::deque<TaskPtr> deque_t;
    deque_t m_deque;
    unsigned m_nthreads;
    unsigned m_waiting;

public:
    TaskQueue();

    /** Needs to know how many threads so it can calculate Count. If you never
     * call this, assumes 1.
     */
    void SetNThreads(unsigned nthreads) { m_nthreads = nthreads; }

    void PushTask(TaskPtr);
    TaskPtr PopTask();
    bool AnyWaiting();
    size_t Count();
};

} // namespace util

#endif
