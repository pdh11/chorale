#include "poll.h"
#include "poller_core.h"
#include "locking.h"
#include "bind.h"
#include <map>
#include <queue>
#include <boost/thread/mutex.hpp>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include "trace.h"

LOG_DECL(POLL);

namespace util {

using pollapi::PollerCore;
using pollapi::PollWaker;

class Poller::Impl: public PerObjectRecursiveLocking,
		    public PollerInterface
{
    
    typedef std::vector<PollRecord> pollables_t;

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

    pollables_t m_pollables;
    timers_t m_timers; // Used as a priority-queue

    PollerCore m_core;

    bool m_array_valid;

    /** Wake up the poll every time a new handle is added */
    PollWaker m_waker;

public:
    Impl();
    ~Impl();

    void Add(Pollable*, const Callback&, unsigned int);
    void Remove(Pollable*);

    void Add(time_t first, unsigned int repeatms, Timed*);
    void Remove(Timed*);

    unsigned Poll(unsigned int timeout_ms);
    void Wake();
};

Poller::Impl::Impl()
    : m_array_valid(false),
      m_waker(this)
{
}

Poller::Impl::~Impl()
{
}

#undef IN
#undef OUT

void Poller::Impl::Add(Pollable *p, const Callback& callback,
		       unsigned int direction)
{
    Lock lock(this);

    if (direction < 1 || direction > 3)
    {
	TRACE << "Bogus direction " << direction << "\n";
	return;
    }

    PollRecord r;
    r.p = p;
    r.c = callback;
    r.h = (direction == IN) ? p->GetReadHandle() : p->GetWriteHandle();
    r.direction = direction;

    if (r.h == NOT_POLLABLE)
    {
	TRACE << "** Warning, attempt to poll non-pollable object\n";
	return;
    }

#if 0
    for (unsigned int i=0; i<m_pollables.size(); ++i)
    {
	if (m_pollables[i].p == p)
	{
	    TRACE << "Pollable " << p << " polled-on twice\n";
	}
	if (m_pollables[i].c == callback)
	{
	    TRACE << "Same callback for two fds: " << r.h << " and "
		  << m_pollables[i].h << "\n";
	}
    }
#endif

    m_pollables.push_back(r);

    m_array_valid = false;

    Wake();
}

void Poller::Impl::Remove(Pollable *p)
{
    Lock lock(this);

    LOG(POLL) << "Remove pollable " << p << "\n";

    for (pollables_t::iterator i = m_pollables.begin();
	 i != m_pollables.end();
	 ++i)
    {
	if (i->p == p)
	{
	    LOG(POLL) << "Found pollable, erasing\n";
	    m_pollables.erase(i);
	    break;
	}
    }
    m_array_valid = false;

    Wake();
}

void Poller::Impl::Add(time_t first, unsigned int repeatms, Timed *callback)
{
    Timer t;
    t.t = (uint64_t)first*1000u;
    t.repeatms = repeatms;
    t.callback = callback;
    
    Lock lock(this);
    m_timers.push_back(t);
//    TRACE << "Pushing timer for " << t.t << "\n";
    std::push_heap(m_timers.begin(), m_timers.end(), TimerSooner());
}

void Poller::Impl::Remove(Timed *callback)
{
    Lock lock(this);
    for (timers_t::iterator i = m_timers.begin(); i != m_timers.end(); ++i)
	if (i->callback == callback)
	    i->callback = NULL;
}

unsigned Poller::Impl::Poll(unsigned int timeout_ms)
{
    {
	Lock lock(this);

	if (!m_array_valid)
	{
	    m_core.SetUpArray(&m_pollables);
	    m_array_valid = true;
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

//		TRACE << "Adjusted timeout to " << timeout_ms << " nowms="
//		      << nowms << "\n";

		// Wake up every ten minutes anyway, just in case
		if (timeout_ms > 600000)
		    timeout_ms = 600000;
	    }
	}
    }

    unsigned int rc = m_core.Poll(timeout_ms);

    if (rc != 0)
	return rc;

    uint64_t nowms = 0;
    bool any = false;
    for (;;)
    {
	Timer t;
	bool doit = false;

	{
	    Lock lock(this);
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

    {
	Lock lock(this);

	m_core.DoCallbacks(&m_pollables, m_array_valid);
    }

    return 0;
}

void Poller::Impl::Wake()
{
    m_waker.Wake();
}

Poller::Poller()
    : m_impl(new Impl)
{
}

Poller::~Poller()
{
    delete m_impl;
}

void Poller::Add(Pollable *p, const Callback& c, unsigned int direction)
{
    m_impl->Add(p, c, direction);
}

void Poller::Remove(Pollable *p)
{
    m_impl->Remove(p);
}

void Poller::Add(time_t first, unsigned int repeatms, Timed *callback)
{
    m_impl->Add(first, repeatms, callback);
}

void Poller::Remove(Timed *callback)
{
    m_impl->Remove(callback);
}

unsigned Poller::Poll(unsigned int timeout_ms)
{
    return m_impl->Poll(timeout_ms);
}

void Poller::Wake()
{
    return m_impl->Wake();
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
//	    TRACE << "now " << m_count << "\n";
	    return 0;
	}
};

int main()
{
    Test1 t1;
    Test1 t2;
    util::Poller poller;

    poller.Add(0, 4000, &t1);

    time_t t = time(NULL);

    poller.Add(t+14, 0, &t2);

    while (t2.m_count == 0)
    {
	poller.Poll(10000);
    }

    assert(t1.m_count == 4); // 0s 4s 8s 12s
    assert(t2.m_count == 1);

    return 0;
}

#endif
