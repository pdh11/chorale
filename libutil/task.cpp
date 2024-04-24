#include "task.h"
#include "trace.h"
#include "printf.h"

namespace util {

Task::Task()
    : m_name(Printf() << this),
      m_observer(NULL)
{
}

Task::Task(const std::string& name)
    : m_name(name),
      m_observer(NULL)
{
}

void Task::SetObserver(TaskObserver *obs)
{
    RecursiveMutex::Lock lock(m_mutex);
    m_observer = obs;
}

void Task::FireProgress(unsigned num, unsigned denom)
{
    RecursiveMutex::Lock lock(m_mutex);
    if (m_observer)
	m_observer->OnProgress(this, num, denom);
}

void Task::FireError(unsigned int e)
{
    RecursiveMutex::Lock lock(m_mutex);
    if (m_observer)
	m_observer->OnError(this, e);
}

} // namespace util
