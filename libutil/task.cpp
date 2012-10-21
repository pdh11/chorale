#include "task.h"
#include "trace.h"
#include "printf.h"

namespace util {

Task::Task()
    : m_name(Printf() << this),
      m_observer(NULL),
      m_done(false)
{
}

Task::Task(const std::string& name)
    : m_name(name),
      m_observer(NULL),
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
