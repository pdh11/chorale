#ifndef LIBUTIL_POLL_H
#define LIBUTIL_POLL_H 1

#include <string.h>
#include <time.h>

#undef IN
#undef OUT

namespace util {

class Pollable
{
public:
    virtual ~Pollable() {}

    virtual unsigned int OnActivity() = 0;
};

class Timed
{
public:
    virtual ~Timed() {}

    virtual unsigned int OnTimer() = 0;
};

/** Abstract poller/timer interface, implemented by util::Poller and
 * choraleqt::Poller
 */
class PollerInterface
{
public:
    virtual ~PollerInterface() {}

    enum { IN = 1, OUT = 2 };

    virtual void AddHandle(int poll_handle, Pollable *callback,
			   unsigned int direction = IN|OUT) = 0;
    virtual void RemoveHandle(int poll_handle) = 0;

    /** Adds a new timer. Timers can be single-shot or repeating.
     *
     * @param first The time at which the callback should happen, or 0 for
     *              straightaway.
     * @param repeatms The timer repeat rate in milliseconds, or 0 for single
     *              shot.
     * @param callback The callback.
     */
    virtual void AddTimer(time_t first, unsigned int repeatms,
			  Timed *callback) = 0;
    virtual void RemoveTimer(Timed*) = 0;
};

/** Implementation of PollerInterface suitable for the main loop of a thread.
 *
 * For instance, the main loop of the choraled daemon consists only of repeated
 * calls to Poller::Poll().
 */
class Poller: public PollerInterface
{
    class Impl;
    Impl *m_impl;
    
public:
    Poller();
    ~Poller();

    enum { INFINITE_MS = (unsigned int)-1 };

    unsigned Poll(unsigned int timeout_ms);

    // Being a PollerInterface
    void AddHandle(int poll_handle, Pollable *callback, 
		   unsigned int direction = IN);
    void RemoveHandle(int poll_handle);

    void AddTimer(time_t first, unsigned int repeatms, Timed*);
    void RemoveTimer(Timed*);
};

/** For explicitly waking-up a Poller
 */
class PollWaker: private Pollable
{
    int m_fd[2];
    Pollable *m_pollable;

    unsigned OnActivity();

public:
    /** Construct a PollWaker for waking up the given PollerInterface. 
     *
     * If the given Pollable is non-NULL, its OnActivity is called at
     * wake time too.
     */
    PollWaker(PollerInterface*, Pollable* = NULL);
    ~PollWaker();
    
    void Wake();
};

} // namespace utilx

#endif
