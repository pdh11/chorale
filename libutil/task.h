#ifndef TASK_H
#define TASK_H

#include "counted_object.h"
#include <stddef.h>
#include "bind.h"
#include "counted_pointer.h"

namespace util {

template <class T> class CountedPointer;

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
class Task: public util::CountedObject<util::PerObjectRecursiveLocking>
{
    TaskObserver *m_observer;
    bool m_done;

protected:
    void FireProgress(unsigned num, unsigned denom);
    void FireError(unsigned int error);

public:
    Task();
    virtual ~Task() {}
    virtual unsigned int Run() = 0; ///< Remove this once TaskCallback is everywhere
    virtual void Cancel() {} // NYI
    void SetObserver(TaskObserver*);
};

typedef util::CountedPointer<Task> TaskPtr;


typedef PtrCallback<TaskPtr> TaskCallback;

template <class T, unsigned (T::*FN)()>
inline TaskCallback Bind(CountedPointer<T> t)
{
    return util::BindPtr<TaskPtr,T,FN>(t); 
}

/** Something to which tasks can be pushed (most likely a pool of background
 * threads).
 */
class TaskQueue
{
public:
    virtual ~TaskQueue() {}

    virtual void PushTask(const TaskCallback&) = 0;
    virtual bool AnyWaiting() = 0;
    virtual size_t Count() = 0;
};

} // namespace util

#endif
