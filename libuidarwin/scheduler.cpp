#include "scheduler.h"
#include "libutil/trace.h"
#include <sys/time.h>

namespace ui {
namespace darwin {


/* Timer */


Timer::Timer(Scheduler *scheduler, time_t first, unsigned int repeatms, 
	     util::TaskCallback callback)
    : m_scheduler(scheduler),
      m_callback(callback),
      m_repeat_ms(repeatms)
{
//    timeval now;
//    ::gettimeofday(&now, NULL);
//    uint64_t nowms = now.tv_sec*(uint64_t)1000 + now.tv_usec/1000;

//    uint64_t firstms = first*(uint64_t)1000;

    // Quixotic floating-point seconds since 2001
    CFAbsoluteTime firstat = (double)first + kCFAbsoluteTimeIntervalSince1970;

    CFRunLoopTimerContext context = { 0, this, NULL, NULL, NULL };

    // We "Create" it, so we must CFRelease it later
    m_timer_ref = CFRunLoopTimerCreate(NULL, firstat,
				       repeatms/1000.0,
				       0, 0, 
				       &Timer::StaticCallback,
				       &context);

    if (m_timer_ref)
	CFRunLoopAddTimer(m_scheduler->GetRunLoop(), m_timer_ref,
			  kCFRunLoopCommonModes);
}

Timer::~Timer()
{
    if (m_timer_ref)
    {
	CFRunLoopRemoveTimer(m_scheduler->GetRunLoop(), m_timer_ref,
			     kCFRunLoopCommonModes);
	CFRunLoopTimerInvalidate(m_timer_ref);
	CFRelease(m_timer_ref);
    }
}

void Timer::StaticCallback(CFRunLoopTimerRef, void *info)
{
    Timer *timer = (Timer*)info;
    timer->OnTimer();
}

void Timer::OnTimer()
{
    if (!m_repeat_ms)
	m_scheduler->RemoveTimer(m_callback.GetPtr().get());

    m_callback();

    if (!m_repeat_ms)
	delete this;
}


/* SocketNotifier */


SocketNotifier::SocketNotifier(int fd,
			       unsigned int cbtype,
			       Scheduler *scheduler, 
			       const util::TaskCallback& callback,
			       bool oneshot)
    : m_pollable(pollable),
      m_scheduler(scheduler),
      m_callback(callback),
      m_oneshot(oneshot)
{
    memset(&m_socket_ctx, '\0', sizeof(m_socket_ctx));
    m_socket_ctx.info = (void*)this;
    m_socket_ref = CFSocketCreateWithNative(NULL,
					    pollable->GetHandle(),
					    cbtype,
					    &StaticCallback,
					    &m_socket_ctx);

    TRACE << "Created CFSocket " << m_socket_ref << " for socket "
	  << pollable->GetHandle() << " flags=" << cbtype << "\n";

    int socketflags = 0;
    if (cbtype & kCFSocketReadCallBack)
	socketflags |= kCFSocketAutomaticallyReenableReadCallBack;

    // Don't close underlying socket on CFSocketInvalidate
    CFSocketSetSocketFlags(m_socket_ref, socketflags);

    m_source_ref = CFSocketCreateRunLoopSource(NULL, m_socket_ref, 0);
    if (m_source_ref)
	CFRunLoopAddSource(m_scheduler->GetRunLoop(), m_source_ref,
			   kCFRunLoopCommonModes);
}

SocketNotifier::~SocketNotifier()
{
    CFRunLoopRemoveSource(m_scheduler->GetRunLoop(), m_source_ref,
			  kCFRunLoopCommonModes);
    CFRelease(m_source_ref);

    TRACE << "Invalidating CFSocket " << m_socket_ref << "\n";

    CFSocketInvalidate(m_socket_ref);
    CFRelease(m_socket_ref);
}

void SocketNotifier::StaticCallback(CFSocketRef,
				    CFSocketCallBackType,
				    CFDataRef,
				    const void*,
				    void *info)
{
    SocketNotifier *notifier = (SocketNotifier*)info;
    notifier->OnActivity(1);
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
    : m_run_loop(CFRunLoopGetCurrent())
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

void Scheduler::RemoveSocketNotifier(util::Pollable *pollable)
{
    util::Mutex::Lock lock(m_mutex);
    m_notifiermap.erase(pollable);
}

void Scheduler::Wait(const util::TaskCallback& callback, time_t first,
		     unsigned int repeatms)
{
    m_timermap[callback.GetPtr().get()] = new Timer(this, first, repeatms, 
						    callback);
}

void Scheduler::WaitForReadable(const util::TaskCallback& callback,
				util::Pollable *pollable, bool oneshot)
{
    m_notifiermap[pollable] =
	new SocketNotifier(pollable,
			   kCFSocketReadCallBack,
			   this, callback,
			   oneshot);
}

void Scheduler::WaitForWritable(const util::TaskCallback& callback,
				util::Pollable *pollable, bool oneshot)
{
    /* Unlike timers, Qt (4.4) doesn't support QSocketNotifiers
     * constructed on the wrong thread.
     */
    CheckThread();

    m_notifiermap[pollable] =
	new SocketNotifier(pollable, 
			   kCFSocketConnectCallBack | kCFSocketWriteCallBack,
			   this, callback,
			   oneshot);
}

} // namespace ui::darwin
} // namespace ui
