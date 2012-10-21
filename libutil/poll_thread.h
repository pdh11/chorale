#ifndef LIBUTIL_POLL_THREAD_H
#define LIBUTIL_POLL_THREAD_H

#include "poll.h"
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

namespace util {

class PollThread: public util::Poller
{
    volatile bool m_exiting;
    boost::thread m_thread;
    boost::mutex m_mutex;
    boost::condition m_exited;

    void Run();

public:
    PollThread();
    ~PollThread();
};

} // namesapce util

#endif
