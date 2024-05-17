#ifndef LIBUTIL_SCHEDULER_H
#define LIBUTIL_SCHEDULER_H

#include <stdint.h>
#include <time.h>
#include "task_queue.h"

namespace util {

class Pollable;
class WorkerThreadPool;

/** Interface for schedulers. 
 *
 * A Scheduler holds a collection of tasks, each waiting for either a timer,
 * some IO, or some CPU.
 *
 * Considerations:
 * 
 * A task needs to sleep on a subsystem (eg UPnP) but only the
 * subsystem knows whether to wait for reading or writing. (So, be
 * able to wait for a task. But then how does result come back
 * typesafely? And who owns the task?) Alternatively, pass a
 * TaskCallback into the subsystem, then it can call you back when
 * it's done.
 */
class Scheduler
{
public:
    virtual ~Scheduler() {}

    /** Wait for a Pollable (eg socket) to become readable, then call
     * back on the TaskCallback.
     *
     * @param callback A TaskCallback (a counted-pointer plus callback combo)
     *                 to be called when the poll handle becomes readable
     * @param poll_handle The socket (on Windows) or arbitrary file descriptor
     *                 to wait for
     * @param oneshot  If true (the default), make the callback once when the
     *                 poll handle first becomes ready. If false, keep on making
     *                 the callback whenever the poll handle becomes ready, until
     *                 cancelled by Remove()
     *
     * Note that poll handles are typically level-triggered, i.e. they'll
     * keep firing as long as they stay readable. This means that if
     * your callback punts the real work to a background thread (like
     * util::http::Server does), you mustn't use oneshot=false as your
     * callback will be called incessantly.
     */
    virtual void WaitForReadable(const TaskCallback& callback,
				 int poll_handle,
				 bool oneshot=true) = 0;

    /** Like WaitForReadable, but for writability.
     */
    virtual void WaitForWritable(const TaskCallback&, int poll_handle,
				 bool oneshot=true) = 0;
    
    /** Wait until the specified time, then call the callback.
     */
    virtual void Wait(const TaskCallback&, time_t first, 
		      unsigned int repeatms) = 0;

    /** Remove this task from all waiting lists.
     *
     * Once this function returns, no new timers or poll callbacks
     * will be started for this task, and any executing timer or poll
     * callback will be complete.
     *
     * If your callback punts the work to a background thread, note
     * that there is no synchronisation here against the background
     * thread -- i.e. it may be the case that your callback has
     * already run, but the background thread hasn't got around to
     * performing the punted work yet.
     */
    virtual void Remove(TaskPtr) = 0;

    virtual void Wake() {}
    virtual void Shutdown() {}
};


/** A Scheduler that runs when you Poll() it.
 *
 * Similar functionality was formerly available as "TaskPoller".
 */
class BackgroundScheduler: public Scheduler
{
    class Impl;
    Impl *m_impl;

public:
    BackgroundScheduler();
    ~BackgroundScheduler();

    enum { INFINITE_MS = (unsigned int)-1 };

    unsigned int Poll(unsigned int ms);
    bool IsExiting() const;

    // Being a Scheduler
    void WaitForReadable(const TaskCallback&, int, bool oneshot) override;
    void WaitForWritable(const TaskCallback&, int, bool oneshot) override;
    void Wait(const TaskCallback&, time_t first,
              unsigned int repeatms) override;
    void Remove(TaskPtr) override;
    void Wake() override;
    void Shutdown() override;
};

} // namespace util

#endif
