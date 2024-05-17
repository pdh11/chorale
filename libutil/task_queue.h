#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stddef.h>

namespace util {

template <class T> class CountedPointer;
template <class T> class PtrCallback;

class Task;

typedef util::CountedPointer<Task> TaskPtr;
typedef PtrCallback<TaskPtr> TaskCallback;

/** Something to which tasks can be pushed (most likely a pool of background
 * threads).
 */
class TaskQueue
{
public:
    virtual ~TaskQueue() {}

    virtual void PushTask(const TaskCallback&) = 0; ///< Back of queue
    virtual void PushTaskFront(const TaskCallback&) = 0;
    virtual bool AnyWaiting() = 0;
    virtual size_t Count() = 0;
};

} // namespace util

#endif
