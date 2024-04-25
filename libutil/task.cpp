#include "task.h"
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
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_observer = obs;
}

void Task::FireProgress(unsigned num, unsigned denom)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_observer)
	m_observer->OnProgress(this, num, denom);
}

void Task::FireError(unsigned int e)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_observer)
	m_observer->OnError(this, e);
}

} // namespace util
