#ifndef LIBUIDARWIN_SCHEDULER_H
#define LIBUIDARWIN_SCHEDULER_H

#include "config.h"
#include "libutil/scheduler.h"
#include "libutil/not_thread_safe.h"
#include <map>

#if HAVE_COREFOUNDATION_CFRUNLOOP_H

#include <CoreFoundation/CFRunLoop.h>
#include <CFNetwork/CFNetwork.h>

/** User-interface classes */
namespace ui {

/** User-interface classes for Darwin-based user interfaces */
namespace darwin {

class Scheduler;

class Timer
{
    Scheduler *m_scheduler;
    util::TaskCallback m_callback;
    unsigned int m_repeat_ms;
    CFRunLoopTimerRef m_timer_ref;

    static void StaticCallback(CFRunLoopTimerRef, void*);

    void OnTimer();

public:

    Timer(Scheduler *parent, time_t first, unsigned int repeatms,
	  util::TaskCallback cb);
    ~Timer();
};

class SocketNotifier
{
    CFSocketRef m_socket_ref;
    CFSocketContext m_socket_ctx;
    CFRunLoopSourceRef m_source_ref;
    util::Pollable *m_pollable;
    Scheduler *m_scheduler;
    util::TaskCallback m_callback;
    bool m_oneshot;

    static void StaticCallback(CFSocketRef s,
			       CFSocketCallBackType callbackType,
			       CFDataRef address,
			       const void *data,
			       void *info);

    void OnActivity(int);

public:
    SocketNotifier(util::Pollable*, unsigned int cbtype,
		   Scheduler *scheduler, const util::TaskCallback& callback,
		   bool oneshot);
    ~SocketNotifier();

    util::TaskPtr GetTask() const { return m_callback.GetPtr(); }
};

/** Implementation of util::Scheduler in terms of Darwin CFRunLoop.
 *
 * Pleasingly, we can do it all in proper C/C++ without any of this
 * Objective-C malarkey.
 */
class Scheduler: public util::Scheduler, public util::NotThreadSafe
{
    CFRunLoopRef m_run_loop;

    util::Mutex m_mutex;
    typedef std::map<util::Task*, Timer*> timermap_t;
    timermap_t m_timermap;
    typedef std::map<util::Pollable*, SocketNotifier*> notifiermap_t;
    notifiermap_t m_notifiermap;

public:
    Scheduler();
    ~Scheduler();

    void RemoveTimer(util::Task*);
    void RemoveSocketNotifier(util::Pollable*);

    CFRunLoopRef GetRunLoop() { return m_run_loop; }

    // Being a Scheduler
    void WaitForReadable(const util::TaskCallback&, util::Pollable*, 
			 bool oneshot);
    void WaitForWritable(const util::TaskCallback&, util::Pollable*,
			 bool oneshot);
    void Wait(const util::TaskCallback&, time_t first, unsigned int repeatms);
    void Remove(util::TaskPtr);
};

} // namespace ui::darwin
} // namespace ui

#endif // HAVE_COREFOUNDATION_CFRUNLOOP_H

#endif
