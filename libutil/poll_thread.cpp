#include "poll_thread.h"
#include "trace.h"
#include <boost/bind.hpp>

namespace util {

PollThread::PollThread()
    : m_exiting(false),
      m_thread(boost::bind(&util::PollThread::Run, this))
{
}

PollThread::~PollThread()
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_exiting = true;
    Wake();
    m_exited.wait(lock);
}

void PollThread::Run()
{
    while (!m_exiting)
    {
	unsigned int rc = Poll(INFINITE_MS);
	if (rc)
	{
	    TRACE << "Poll failed (" << rc << "), exiting\n";
	}
    }

    boost::mutex::scoped_lock lock(m_mutex);
    m_exited.notify_all();
}

} // namespace util
