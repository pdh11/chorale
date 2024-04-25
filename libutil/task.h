#ifndef TASK_H
#define TASK_H

#include "counted_object.h"
#include <mutex>
//#include <stddef.h>
#include <string>

namespace util {

class TaskObserver;

/** A Task is a unit of work suitable for handing off to a background
 * thread. It can send progress (but the progress callback is made on
 * the background thread). To hand off a Task to a WorkerThread, push
 * it on the WorkerThread's TaskQueue.
 */
class Task: public util::CountedObject
{
    std::string m_name;
    std::recursive_mutex m_mutex; // protects m_observer
    TaskObserver *m_observer;

protected:
    void FireProgress(unsigned num, unsigned denom);
    void FireError(unsigned int error);

public:
    Task();
    explicit Task(const std::string& name);

    virtual ~Task() {}
    virtual unsigned int Run() = 0; ///< Remove this once TaskCallback is everywhere
    virtual void Cancel() {} // NYI
    void SetObserver(TaskObserver*);

    const std::string& Name() const { return m_name; }
};

} // namespace util

#endif
