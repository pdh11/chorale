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
    boost::mutex::scoped_lock lock(m_observer_mutex);
    m_observer = obs;
}

void Task::FireProgress(unsigned num, unsigned denom)
{
    boost::mutex::scoped_lock lock(m_observer_mutex);
    if (m_observer)
	m_observer->OnProgress(this, num, denom);
}

void Task::FireError(unsigned int e)
{
    boost::mutex::scoped_lock lock(m_observer_mutex);
    if (m_observer)
	m_observer->OnError(this, e);
}

} // namespace util
