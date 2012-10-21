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


TaskQueue::TaskQueue()
    : m_nthreads(1),
      m_waiting(0)
{
}

void TaskQueue::PushTask(TaskPtr t)
{
    boost::mutex::scoped_lock lock(m_deque_mutex);
    m_deque.push_back(t);
    m_dequenotempty.notify_one();
}

TaskPtr TaskQueue::PopTask()
{
    boost::mutex::scoped_lock lock(m_deque_mutex);

    ++m_waiting;
    while (m_deque.empty())
    {
//	TRACE << m_waiting << "waiting\n";
	m_dequenotempty.wait(lock);
//	TRACE << "waited\n";
    }
    --m_waiting;
    TaskPtr result = m_deque.front();
    m_deque.pop_front();
    return result;
}

bool TaskQueue::AnyWaiting()
{
    boost::mutex::scoped_lock lock(m_deque_mutex);
    return m_waiting > 0;
}

unsigned TaskQueue::Count()
{
    boost::mutex::scoped_lock lock(m_deque_mutex);
    return m_deque.size() + m_nthreads - m_waiting;
}

}; // namespace util
