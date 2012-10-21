#include "pollable_task.h"
#include "poll.h"
#include "bind.h"
#include "trace.h"
#include "worker_thread_pool.h"

LOG_DECL(POLL);

#undef IN
#undef OUT

namespace util {

class TaskPoller::WaitingTask: public util::Timed
{
    TaskPoller *m_parent;
    TaskPtr m_task;
    Pollable *m_pollable;

public:
    WaitingTask(TaskPoller *parent, TaskPtr task, Pollable *pollable)
	: m_parent(parent),
	  m_task(task),
	  m_pollable(pollable)
    {
//	TRACE << "wt " << this << " t" << m_task.get() << ": WT()\n";
    }

    ~WaitingTask()
    {
//	TRACE << "wt" << this << ": ~WaitingTask\n";
	if (m_pollable)
	    m_parent->m_poller->Remove(m_pollable);
	m_parent->m_poller->Remove(this);
//	TRACE << "wt" << this << ": ~WaitingTask done\n";
    }

    unsigned int OnActivity()
    {
//	TRACE << "wt" << this << " t" << m_task.get() << ": OnActivity\n";
	if (m_parent->m_exiting)
	    return 0;

	m_parent->m_poller->Remove(m_pollable);
	RunMe();
	return 0;
    }

    unsigned int OnTimer()
    {
	if (m_parent->m_exiting)
	    return 0;

	m_parent->m_poller->Remove(this);
	RunMe();
	return 0;
    }

    void RunMe()
    {
	{
	    TaskPoller::Lock lock(m_parent);
	    m_parent->m_waiting_tasks.erase(this);
	}
	m_parent->m_thread_pool->PushTask(m_task);
	delete this;
    }
};

TaskPoller::TaskPoller(PollerInterface *poller,
		       WorkerThreadPool *thread_pool)
    : m_poller(poller),
      m_thread_pool(thread_pool),
      m_exiting(false)
{
}

TaskPoller::~TaskPoller()
{
    LOG(POLL) << "~TaskPoller\n";

    m_exiting = true;

    {
	Lock lock(this);
	
	while (!m_waiting_tasks.empty())
	{
	    std::set<WaitingTask*>::iterator i = m_waiting_tasks.begin();
	    WaitingTask *p = *i;
	    m_waiting_tasks.erase(i);
	    delete p;
	}
    }

    LOG(POLL) << "~TaskPoller calls Shutdown\n";

    /* If we leave any threads alive, they might call back into us. So we must
     * synchronously close the pool before leaving this destructor.
     */
    m_thread_pool->Shutdown();

    LOG(POLL) << "~TaskPoller: Shutdown succeeded\n";
}

void TaskPoller::Wait(TaskPtr ptr, Pollable *pollable, unsigned int direction)
{
    if (m_exiting)
	return;

    WaitingTask *task = new WaitingTask(this, ptr, pollable);
    {
	Lock lock(this);
	if (m_exiting)
	{
	    delete task;
	    return;
	}

	m_waiting_tasks.insert(task);
    }
//    TRACE << "Wait adds " << poll_handle << "\n";
    m_poller->Add(pollable, Bind<WaitingTask, &WaitingTask::OnActivity>(task),
		  direction);
}

void TaskPoller::Wait(TaskPtr ptr, time_t timer)
{
    if (m_exiting)
	return;

    WaitingTask *task = new WaitingTask(this, ptr, NULL);
    {
	Lock lock(this);
	if (m_exiting)
	{
	    delete task;
	    return;
	}
	m_waiting_tasks.insert(task);
    }
    m_poller->Add(timer, 0, task);
}

void PollableTask::WaitForReadable(Pollable *p)
{
    m_poller->Wait(TaskPtr(this), p, PollerInterface::IN);
}

void PollableTask::WaitForWritable(Pollable *p)
{
    m_poller->Wait(TaskPtr(this), p, PollerInterface::OUT);
}

void PollableTask::WaitForTimer(time_t t)
{
    m_poller->Wait(TaskPtr(this), t);
}

} // namespace util

#ifdef TEST

# include "magic.h"
# include "worker_thread_pool.h"

#ifndef WIN32
volatile bool done = false;

struct StandardOutput: public util::Pollable
{
    int GetWriteHandle() { return 1; }
};

class TwoShots: public util::Task, public util::Magic<0x22222222>
{
    util::TaskPoller *m_task_poller;
    enum {
	FIRST,
	SECOND
    } m_state;
    
    StandardOutput m_sout;

public:
    TwoShots(util::TaskPoller *tp) : m_task_poller(tp), m_state(FIRST) {}
    ~TwoShots();

    unsigned int Run();
};

TwoShots::~TwoShots()
{
//    TRACE << "~TwoShots\n";
}

unsigned int TwoShots::Run()
{
//    TRACE << "Running in state " << m_state << "\n";
    AssertValid();

    switch (m_state)
    {
    case FIRST:
	m_state = SECOND;
	// Wait for stdout to be writable
	m_task_poller->Wait(util::TaskPtr(this), &m_sout,
			    util::PollerInterface::OUT);
	return 0;
    case SECOND:
	done = true;
	return 0;
    }

    return 0;
}
#endif

int main()
{
#ifndef WIN32
    util::Poller poller;
    util::WorkerThreadPool wt(util::WorkerThreadPool::NORMAL);

    util::TaskPoller tp(&poller, &wt);

    wt.PushTask(util::TaskPtr(new TwoShots(&tp)));

    do {
//	TRACE << "Polling\n";
	poller.Poll(100);
    } while (!done);

//    TRACE << "Done\n";
#endif
    return 0;
}

#endif
