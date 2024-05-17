#include "scheduler_task.h"
#include "trace.h"
#include "scheduler.h"
#include "bind.h"
#include "counted_pointer.h"

LOG_DECL(POLL);

namespace util {

SchedulerTask::SchedulerTask(BackgroundScheduler *scheduler)
    : m_scheduler(scheduler)
{
}

unsigned SchedulerTask::Run()
{
    while (!m_scheduler->IsExiting())
    {
	unsigned rc = m_scheduler->Poll(BackgroundScheduler::INFINITE_MS);
	LOG(POLL) << "Awake, rc=" << rc << ", exiting="
		  << m_scheduler->IsExiting() << "\n";
	if (rc)
	    return rc;
    }
    return 0;
}

TaskCallback SchedulerTask::Create(BackgroundScheduler *scheduler)
{
    CountedPointer<SchedulerTask> task(new SchedulerTask(scheduler));
    return Bind(task).To<&SchedulerTask::Run>();
}

} // namespace util
