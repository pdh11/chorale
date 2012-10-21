#include "poll.h"
#include <map>
#include <queue>
#include <boost/thread/mutex.hpp>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include "trace.h"

namespace util {

using pollapi::PollerCore;

class Poller::Impl
{
    typedef std::map<int, Pollable*> handles_t;
    typedef std::map<int, unsigned int> directions_t;

    struct Timer
    {
	uint64_t t; // time_t * 1000
	unsigned int repeatms;
	Timed *callback;
    };

    /** Comparator for timers that makes queue.top() the soonest.
     *
     * This means that operator()(x,y) returns true if y is sooner.
     */
    struct TimerSooner
    {
	bool operator()(const Timer& x, const Timer& y)
	{
	    return y.t < x.t;
	}
    };

    typedef std::vector<Timer> timers_t;

    boost::mutex m_mutex;    
    handles_t m_handles;
    directions_t m_directions;
    timers_t m_timers; // Used as a priority-queue

    PollerCore m_core;
    bool m_array_valid;

public:
    Impl();
    ~Impl();

    void AddHandle(int, Pollable*, unsigned int);
    void RemoveHandle(int);

    void AddTimer(time_t first, unsigned int repeatms, Timed*);
    void RemoveTimer(Timed*);

    unsigned Poll(unsigned int timeout_ms);
};

Poller::Impl::Impl()
    : //m_array(NULL),
      m_array_valid(false)
{
}

Poller::Impl::~Impl()
{
//    free(m_array);
}

#undef IN
#undef OUT

void Poller::Impl::AddHandle(int fd, Pollable *callback,
			     unsigned int direction)
{
    boost::mutex::scoped_lock lock(m_mutex);

    if (direction < 1 || direction > 3)
    {
	TRACE << "Bogus direction " << direction << "\n";
    }

    m_directions[fd] = direction;
    m_handles[fd] = callback;
    m_array_valid = false;
}

void Poller::Impl::RemoveHandle(int fd)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_directions.erase(fd);
    m_handles.erase(fd);
    m_array_valid = false;
}

void Poller::Impl::AddTimer(time_t first, unsigned int repeatms,
			    Timed *callback)
{
    Timer t;
    t.t = (uint64_t)first*1000u;
    t.repeatms = repeatms;
    t.callback = callback;
    
    boost::mutex::scoped_lock lock(m_mutex);
    m_timers.push_back(t);
//    TRACE << "Pushing timer for " << t.t << "\n";
    std::push_heap(m_timers.begin(), m_timers.end(), TimerSooner());
}

void Poller::Impl::RemoveTimer(Timed *callback)
{
    boost::mutex::scoped_lock lock(m_mutex);
    for (timers_t::iterator i = m_timers.begin(); i != m_timers.end(); ++i)
	if (i->callback == callback)
	    i->callback = NULL;
}

unsigned Poller::Impl::Poll(unsigned int timeout_ms)
{
//    nfds_t nfds;

    {
	boost::mutex::scoped_lock lock(m_mutex);

	if (!m_array_valid)
	{
	    m_core.SetUpArray(&m_handles, &m_directions);
/*
	    struct pollfd *newarray = (struct pollfd*) realloc(m_array, 
					      m_handles.size() * sizeof(struct pollfd));
	    if (!newarray)
		return ENOMEM;
	    m_array = newarray;
	    size_t i = 0;
	    for (handles_t::const_iterator j = m_handles.begin();
		 j != m_handles.end();
		 ++j)
	    {
		m_array[i].fd = j->first;
		short events = 0;
		unsigned int directions = m_directions[j->first];
		if (directions & IN)
		    events |= POLLIN;
		if (directions & OUT)
		    events |= POLLOUT;
//		TRACE << "Waiting on fd " << j->first << " events "
//		      << events << "\n";
		m_array[i].events = events;
		++i;
	    }
	    nfds = (nfds_t)i;
*/
	    m_array_valid = true;
	}
	else
	{
//	    nfds = m_handles.size();
//	    TRACE << "Re-waiting on " << nfds << " fds\n";
	}

	if (!m_timers.empty())
	{
	    timeval now;
	    ::gettimeofday(&now, NULL);
	    uint64_t nowms = (uint64_t)now.tv_sec*1000
		+ (uint64_t)(now.tv_usec/1000);
	    
	    if (nowms >= m_timers.front().t)
		timeout_ms = 0;
	    else if (timeout_ms == Poller::INFINITE_MS
		     || (nowms+timeout_ms) > m_timers.front().t)
	    {
		timeout_ms = (unsigned)(m_timers.front().t - nowms);

		TRACE << "Adjusted timeout to " << timeout_ms << " nowms="
		      << nowms << "\n";

		// Wake up every ten minutes anyway, just in case
		if (timeout_ms > 600000)
		    timeout_ms = 600000;
	    }
	}
    }

//    int rc = ::poll(m_array, nfds, (int)timeout_ms);

//    if (rc < 0)
//	return (unsigned)errno;

    unsigned int rc = m_core.Poll(timeout_ms);

//    TRACE << "polled " << rc << "\n";

    if (rc != 0)
	return rc;

    uint64_t nowms = 0;
    bool any = false;
    for (;;)
    {
	Timer t;
	bool doit = false;

	{
	    boost::mutex::scoped_lock lock(m_mutex);
	    if (m_timers.empty())
		break;

	    if (!any)
	    {
		timeval now;
		::gettimeofday(&now, NULL);
		nowms = (uint64_t)now.tv_sec*1000
		    + (uint64_t)(now.tv_usec/1000);
		any = true;
	    }

	    /* Because we always compare against the same value -- as opposed
	     * to calling gettimeofday each time round the loop -- even a
	     * repeating timer is called at most once per invocation of Poll().
	     */
	    if (m_timers.front().t > nowms)
		break;

	    t = m_timers.front();
//	    TRACE << "now " << nowms << ", timer " << t.t << ", calling\n";

	    doit = true;

	    /* std::pop_heap moves the "popped" element to the end,
	     * where we can pop_back() it to delete it, or push_heap()
	     * it to re-enable it.
	     */
	    std::pop_heap(m_timers.begin(), m_timers.end(), TimerSooner());

	    if (t.repeatms == 0 || !t.callback)
		m_timers.pop_back();
	    else
	    {
		if (t.t == 0)
		{
		    m_timers.back().t = nowms + t.repeatms;
		}
		else
		{
		    // Round up to next scheduled event that's still
		    // in the future
		    m_timers.back().t += ((nowms + t.repeatms - t.t)
					   / t.repeatms)
			                 * t.repeatms;

		    assert(m_timers.back().t > nowms);
		    assert(m_timers.back().t <= (nowms + t.repeatms));
		}

//		TRACE << "Next repeat at " << m_timers.back().t << "\n";
		std::push_heap(m_timers.begin(), m_timers.end(), TimerSooner());
	    }
	}

	if (doit && t.callback)
	    t.callback->OnTimer();
    }

    m_core.DoCallbacks(&m_handles);
/*

    if (rc == 0)
	return 0;

    for (nfds_t i=0; i<nfds; ++i)
    {
	if (m_array[i].revents)
	{
//	    TRACE << "Activity on fd " << m_array[i].fd << "\n";
	    m_handles[m_array[i].fd]->OnActivity();
	}
    }
//    TRACE << "Done activity calls\n";
*/
    return 0;
}

Poller::Poller()
    : m_impl(new Impl)
{
}

Poller::~Poller()
{
    delete m_impl;
}

void Poller::AddHandle(int fd, Pollable *p, unsigned int direction)
{
    m_impl->AddHandle(fd, p, direction);
}

void Poller::RemoveHandle(int fd)
{
    m_impl->RemoveHandle(fd);
}

void Poller::AddTimer(time_t first, unsigned int repeatms, Timed *callback)
{
    m_impl->AddTimer(first, repeatms, callback);
}

void Poller::RemoveTimer(Timed *callback)
{
    m_impl->RemoveTimer(callback);
}

unsigned Poller::Poll(unsigned int timeout_ms)
{
    return m_impl->Poll(timeout_ms);
}

} // namespace util

#ifdef TEST

class Test1: public util::Timed
{
public:
    unsigned int m_count;

    Test1() : m_count(0) {}
    unsigned int OnTimer()
	{
	    ++m_count; 
	    return 0;
	}
};

int main()
{
    Test1 t1;
    Test1 t2;
    util::Poller poller;

    poller.AddTimer(0, 2000, &t1);

    time_t t = time(NULL);

    poller.AddTimer(t+7, 0, &t2);

    while (t2.m_count == 0)
    {
	poller.Poll(10000);
    }

    assert(t1.m_count == 4); // 0s 2s 4s 6s
    assert(t2.m_count == 1);

    return 0;
}

#endif
