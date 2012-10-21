#include "scheduler.h"
#include "bind.h"
#include "locking.h"
#include "errors.h"
#include <stdint.h>
#include <vector>
#include <deque>
#include <queue>
#include <list>
#include <algorithm>
#include <sys/time.h>
#include "task.h"
#include "counted_pointer.h"
#include "poll.h"
#include "trace.h"

LOG_DECL(POLL);

namespace util {

using pollapi::PollerCore;

class BackgroundScheduler::Impl: private util::PerObjectRecursiveLocking
{
    friend class BackgroundScheduler;

    typedef std::vector<PollRecord> pollables_t;

    struct Timed
    {
	uint64_t t; // time_t * 1000
	unsigned int repeatms;
	TaskCallback callback;
    };

    /** Comparator for timers that makes queue.top() the soonest.
     *
     * This means that operator()(x,y) returns true if y is sooner.
     */
    struct TimerSooner
    {
	bool operator()(const Timed& x, const Timed& y)
	{
	    return y.t < x.t;
	}
    };

    typedef std::vector<Timed> timed_t;

    pollables_t m_pollables;
    timed_t m_timers; // Used as a priority-queue

    bool m_array_valid;
    bool m_exiting;

    PollerCore m_core;

public:
    Impl();
    ~Impl();

    unsigned int Poll(unsigned int ms);

    void Wait(const TaskCallback&, Pollable*, unsigned int direction,
	      bool oneshot);
    void Wait(const TaskCallback&, time_t first, unsigned int repeatms);
    void Wait(const TaskCallback&, TaskPtr waitedon);
    void Remove(TaskPtr);
    void Shutdown();
};

BackgroundScheduler::Impl::Impl()
    : m_array_valid(false), m_exiting(false)
{
}

void BackgroundScheduler::Impl::Shutdown()
{
    if (!m_exiting)
    {
	m_exiting = true;

	Lock lock(this);
	
	LOG(POLL) << "BS" << (void*)this << " shutdown wakes\n";
	m_core.Wake();
	m_timers.clear();
	m_pollables.clear();
	LOG(POLL) << "Shutdown done\n";
    }
}

BackgroundScheduler::Impl::~Impl()
{
    Shutdown();
}

#undef IN
#undef OUT

void BackgroundScheduler::Impl::Wait(const TaskCallback& tc, Pollable *p,
				     unsigned int direction, bool oneshot)
{
    if (m_exiting)
	return;

    Lock lock(this);

    if (m_exiting)
	return;

    PollRecord r;
    r.tc = tc;
    r.h = p->GetHandle();
    r.direction = (unsigned char)direction;
    r.oneshot = oneshot;
    r.internal = NULL;

    if (r.h == NOT_POLLABLE)
    {
	TRACE << "** Warning, attempt to poll non-pollable object\n";
	return;
    }

    for (unsigned int i=0; i<m_pollables.size(); ++i)
    {
	if (m_pollables[i].h == r.h && m_pollables[i].direction)
	{
	    TRACE << "Pollable fd " << r.h << " polled-on twice\n";
	    assert(false);
	}
    }

    m_pollables.push_back(r);

    m_array_valid = false;

    m_core.Wake();
}

void BackgroundScheduler::Impl::Remove(TaskPtr p)
{
    if (m_exiting)
	return;

    Lock lock(this);

    LOG(POLL) << "Remove timed " << p << "\n";

    for (timed_t::iterator i = m_timers.begin(); i != m_timers.end(); ++i)
	if (i->callback.GetPtr() == p)
	    i->callback = TaskCallback();

    LOG(POLL) << "Remove pollable " << p << "\n";

    for (pollables_t::iterator i = m_pollables.begin();
	 i != m_pollables.end();
	 ++i)
    {
	if (i->tc.GetPtr() == p)
	{
	    LOG(POLL) << "Found pollable, erasing\n";
	    m_core.Deleting(&*i);
	    m_pollables.erase(i);
	    break;
	}
    }
    m_array_valid = false;

    m_core.Wake();
}

void BackgroundScheduler::Impl::Wait(const TaskCallback& callback,
				     time_t first, unsigned int repeatms)
{
    if (m_exiting)
	return;

    Timed t;
    t.t = (uint64_t)first*1000u;
    t.repeatms = repeatms;
    t.callback = callback;
    
    Lock lock(this);
    if (m_exiting)
	return;

    m_timers.push_back(t);
//    TRACE << "Pushing timer for " << t.t << "\n";
    std::push_heap(m_timers.begin(), m_timers.end(), TimerSooner());

    m_core.Wake();
}

static bool DoneOneShot(const PollRecord& rec)
{
    return !rec.direction;
}

unsigned BackgroundScheduler::Impl::Poll(unsigned int timeout_ms)
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
	    else if (timeout_ms == BackgroundScheduler::INFINITE_MS
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

    LOG(POLL) << "BS" << (void*)this << " calls poll(" << timeout_ms << ")\n";
    unsigned int rc = m_core.Poll(timeout_ms);
    LOG(POLL) << "BS" << (void*)this << " poll returned " << rc << "\n";

    if (rc != 0)
	return rc;

    if (m_exiting)
	return 0;

    uint64_t nowms = 0;
    bool any = false;
    for (;;)
    {
	Timed t;
	bool doit = false;

	{
//	    TRACE << "BS gets timer lock\n";
	    Lock lock(this);
//	    TRACE << "Got timer lock\n";
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

	    /* Because the lock is a recursive lock, we can call the
	     * callback inside it.
	     */
	    if (t.callback)
		t.callback();
	}
    }

    {
//	TRACE << "BS gets lock\n";
	Lock lock(this);

//	TRACE << "BS got lock, does callbacks\n";
	m_core.DoCallbacks(&m_pollables, m_array_valid);
//	TRACE << "BS done callbacks\n";

	pollables_t::iterator it = std::remove_if(m_pollables.begin(),
						  m_pollables.end(),
						  &DoneOneShot);
	if (it != m_pollables.end())
	{
	    m_pollables.erase(it, m_pollables.end());
	    m_array_valid = false;
	}
    }

//    TRACE << "BS returns\n";

    return 0;
}

BackgroundScheduler::BackgroundScheduler()
    : m_impl(new Impl)
{
}

BackgroundScheduler::~BackgroundScheduler()
{
    delete m_impl;
}

void BackgroundScheduler::WaitForReadable(const TaskCallback& callback,
					  Pollable *pollable, bool oneshot)
{
    m_impl->Wait(callback, pollable, PollRecord::IN, oneshot);
}

void BackgroundScheduler::WaitForWritable(const TaskCallback& callback,
					  Pollable *pollable, bool oneshot)
{
    m_impl->Wait(callback, pollable, PollRecord::OUT, oneshot);
}

void BackgroundScheduler::Wait(const TaskCallback& callback, time_t first,
			       unsigned int repeatms)
{
    m_impl->Wait(callback, first, repeatms);
}

unsigned int BackgroundScheduler::Poll(unsigned int ms)
{
    return m_impl->Poll(ms);
}

void BackgroundScheduler::Remove(TaskPtr p)
{
    m_impl->Remove(p);
}

void BackgroundScheduler::Wake()
{
    m_impl->m_core.Wake();
}

void BackgroundScheduler::Shutdown()
{
    m_impl->Shutdown();
}

bool BackgroundScheduler::IsExiting() const
{
    return m_impl->m_exiting;
}


/* SchedulerTask */


SchedulerTask::SchedulerTask(BackgroundScheduler *scheduler)
    : m_scheduler(scheduler)
{
}

unsigned SchedulerTask::Run()
{
    while (!m_scheduler->IsExiting())
    {
	unsigned rc = m_scheduler->Poll(BackgroundScheduler::INFINITE_MS);
	LOG(POLL) << "Awake, rc=" << rc << ", exiting="
		  << m_scheduler->IsExiting() << "\n";
	if (rc)
	    return rc;
    }
    return 0;
}

TaskCallback SchedulerTask::Create(BackgroundScheduler *scheduler)
{
    CountedPointer<SchedulerTask> task(new SchedulerTask(scheduler));
    return Bind<SchedulerTask,&SchedulerTask::Run>(task);
}


} // namespace util

#ifdef TEST

# include "config.h"

unsigned int g_calls = 0;
unsigned int g_destroy = 0;
ssize_t g_rc;

class TestTask: public util::Task
{
public:
    ~TestTask() { ++g_destroy; }

    unsigned Run() { ++g_calls; return 0; }
    unsigned Run1() { return 0; }
    unsigned Run2() { return 0; }
};

typedef util::CountedPointer<TestTask> TestPtr;

class TestPollable: public util::Pollable
{
    int m_fd;

public:
    TestPollable(int fd) : m_fd(fd) {}

    util::PollHandle GetHandle() { return m_fd; }
};

int main()
{
    {
	util::BackgroundScheduler poller;

	time_t start = time(NULL);

	poller.Wait(util::Bind<TestTask,&TestTask::Run>(TestPtr(new TestTask)),
		    start+2, 0);

	time_t finish = start+4;
	time_t now;

	do {
	    now = time(NULL);
	    poller.Poll((unsigned)(finish-now)*1000);
	} while (now < finish && g_calls == 0);

	assert(g_calls == 1);
	assert(g_destroy == 1);

#if HAVE_PIPE
	int pipefd[2];

	if (::pipe(pipefd) < 0)
	{
	    TRACE << "pipe() failed\n";
	    assert(false);
	}

	TestPollable pollable(pipefd[0]);

	poller.WaitForReadable(
	    util::Bind<TestTask,&TestTask::Run>(TestPtr(new TestTask)),
	    &pollable, true);

	g_rc = ::write(pipefd[1], "*", 1);

	poller.Poll(0);

	assert(g_calls == 2);
	assert(g_destroy == 2);

	char buf[1];
	g_rc = read(pipefd[0], buf, 1);

	poller.WaitForReadable(
	    util::Bind<TestTask,&TestTask::Run>(TestPtr(new TestTask)),
	    &pollable, false);

	g_rc = ::write(pipefd[1], "*", 1);

	poller.Poll(0);

	assert(g_calls == 3);
	assert(g_destroy == 2); // i.e. still around

	g_rc = read(pipefd[0], buf, 1);

	g_rc = ::write(pipefd[1], "*", 1);

	poller.Poll(0);
	assert(g_calls == 4);
#endif
    }

#if HAVE_PIPE
    assert(g_destroy == 3);
#else
    assert(g_destroy == 1);
#endif

    return 0;
}

#endif
