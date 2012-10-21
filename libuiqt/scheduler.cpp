#include "scheduler.h"
#include "libutil/trace.h"
#include <sys/time.h>
#include <QApplication>

namespace ui {
namespace qt {


/* Timer */


Timer::Timer(Scheduler *scheduler, unsigned int repeatms, 
	     util::TaskCallback callback)
    : QTimer(),
      m_scheduler(scheduler),
      m_repeatms(repeatms),
      m_callback(callback),
      m_first(true)
{
    connect(this, SIGNAL(timeout()), this, SLOT(OnTimer()));
    setSingleShot(true);
}

Timer::~Timer()
{
    stop();
}

void Timer::OnTimer()
{
    if (m_first && m_repeatms)
    {
	setSingleShot(false);
	start(m_repeatms);
	m_first = false;
    }

    util::TaskCallback cb = m_callback;
    
    if (!m_repeatms)
    {
	m_scheduler->RemoveTimer(m_callback.GetPtr().get());
	delete this;
    }

    cb();
}


/* SocketNotifier */


SocketNotifier::SocketNotifier(int pollable,
			       QSocketNotifier::Type direction,
			       Scheduler *scheduler, 
			       const util::TaskCallback& callback,
			       bool oneshot)
    : QSocketNotifier(pollable, direction),
      m_pollable(pollable),
      m_scheduler(scheduler),
      m_callback(callback),
      m_oneshot(oneshot)
{
    connect(this, SIGNAL(activated(int)), this, SLOT(OnActivity(int)));
}

SocketNotifier::~SocketNotifier()
{
}

void SocketNotifier::OnActivity(int)
{
    if (m_oneshot)
	m_scheduler->RemoveSocketNotifier(m_pollable);

    m_callback();

    if (m_oneshot)
	delete this;
}


/* Scheduler */


Scheduler::Scheduler()
{
}

Scheduler::~Scheduler()
{
}

void Scheduler::Remove(util::TaskPtr task)
{
    util::Mutex::Lock lock(m_mutex);
    timermap_t::iterator it = m_timermap.find(task.get());
    if (it != m_timermap.end())
    {
	it->second->stop();
	delete it->second;
	m_timermap.erase(it);
    }
    for (notifiermap_t::iterator i = m_notifiermap.begin();
	 i != m_notifiermap.end(); 
	 /*skip*/ )
    {
	if (i->second->GetTask() == task)
	{
	    delete i->second;
	    m_notifiermap.erase(i++); // Not ++i!
	}
	else
	    ++i;
    }
}

void Scheduler::RemoveTimer(util::Task *task)
{
    util::Mutex::Lock lock(m_mutex);
    m_timermap.erase(task);
}

void Scheduler::RemoveSocketNotifier(int pollable)
{
    util::Mutex::Lock lock(m_mutex);
    m_notifiermap.erase(pollable);
}

void Scheduler::Wait(const util::TaskCallback& callback, time_t first,
		     unsigned int repeatms)
{
    timeval now;
    ::gettimeofday(&now, NULL);
    uint64_t nowms = now.tv_sec*(uint64_t)1000 + now.tv_usec/1000;

    uint64_t firstms = first*(uint64_t)1000;

    util::Mutex::Lock lock(m_mutex);
    Timer* t = new Timer(this, repeatms, callback);
    t->moveToThread(QApplication::instance()->thread());
    m_timermap[callback.GetPtr().get()] = t;
    t->start((firstms > nowms) ? (int)(firstms - nowms) : 0);
}

void Scheduler::WaitForReadable(const util::TaskCallback& callback,
				int pollable, bool oneshot)
{
    /* Unlike timers, Qt (4.4) doesn't support QSocketNotifiers
     * constructed on the wrong thread.
     */
    CheckThread();

    m_notifiermap[pollable] =
	new SocketNotifier(pollable, QSocketNotifier::Read, this, callback,
			   oneshot);
}

void Scheduler::WaitForWritable(const util::TaskCallback& callback,
				int pollable, bool oneshot)
{
    /* Unlike timers, Qt (4.4) doesn't support QSocketNotifiers
     * constructed on the wrong thread.
     */
    CheckThread();

    m_notifiermap[pollable] =
	new SocketNotifier(pollable, QSocketNotifier::Write, this, callback,
			   oneshot);
}

} // namespace ui::qt
} // namespace ui

#ifdef TEST

bool g_done = false;
bool g_dead = false;

struct Test: public util::Task
{
    Test() {}
    ~Test() { g_dead = true; }
    
    unsigned Run() { g_done = true; return 0; }
};

typedef util::CountedPointer<Test> TestPtr;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ui::qt::Scheduler poller;

    time_t start = time(NULL);

    poller.Wait(util::Bind(TestPtr(new Test)).To<&Test::Run>(),
		start+2, 0);

    time_t finish = start+4;
    time_t now;

    do {
	now = time(NULL);
	app.processEvents(QEventLoop::AllEvents, (int)(finish-now)*1000);
    } while (now < finish && !g_done);

    assert(g_done);
    assert(g_dead);

#ifndef WIN32

    g_done = g_dead = false;

    int pipefd[2];

    if (::pipe(pipefd) < 0)
    {
	TRACE << "pipe() failed\n";
	assert(false);
    }

    poller.WaitForReadable(util::Bind(TestPtr(new Test)).To<&Test::Run>(), 
			   pipefd[0], true);

    ssize_t rc = ::write(pipefd[1], "*", 1);
    if (rc != 1)
	assert(false);

    app.processEvents();

    assert(g_done);
    assert(g_dead);
#endif

    return 0;
}

#endif
