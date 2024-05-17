#ifndef LIBUTIL_SCHEDULER_TASK_H
#define LIBUTIL_SCHEDULER_TASK_H

#include "task.h"
#include "task_queue.h"

namespace util {

class BackgroundScheduler;

/** For when you want a Scheduler to be polled from a background thread.
 *
 * Create one of these and push it to a task queue to get polling done
 * on a (single, consistent) background thread.
 */
class SchedulerTask: public Task
{
    BackgroundScheduler *m_scheduler;

    explicit SchedulerTask(BackgroundScheduler*);
    unsigned Run() override;

public:
    static TaskCallback Create(BackgroundScheduler*);
};

} // namespace util

#endif
