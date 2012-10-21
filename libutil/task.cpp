#include "task.h"
#include "trace.h"

namespace util {

Task::Task()
    : m_observer(NULL),
      m_done(false)
{
}

void Task::SetObserver(TaskObserver *obs)
{
    Lock lock(this);
    m_observer = obs;
}

void Task::FireProgress(unsigned num, unsigned denom)
{
    Lock lock(this);
    if (m_observer)
	m_observer->OnProgress(this, num, denom);
}

void Task::FireError(unsigned int e)
{
    Lock lock(this);
    if (m_observer)
	m_observer->OnError(this, e);
}

} // namespace util
