#ifndef TASK_OBSERVER_H
#define TASK_OBSERVER_H

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

} // namespace util

#endif
