#ifndef LIBUTIL_POLL_H
#define LIBUTIL_POLL_H 1

#include <time.h>
#include <boost/intrusive_ptr.hpp>
#include "pollable.h"

#undef IN
#undef OUT

struct pollfd;

namespace util {

class Callback;

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

    /** Add a handle to the list being polled for.
     */
    virtual void Add(Pollable*, const Callback& callback,
		     unsigned int direction = IN|OUT) = 0;

    /** Because so many Pollables are CountedPointers, provide an overload.
     *
     * Only compiles if T.get() is a subclass of Pollable.
     */
    template <class T>
    void Add(boost::intrusive_ptr<T>& ptr, const Callback& c,
	     unsigned int d)
    {
	Add(ptr.get(), c, d);
    }

    /** Remove a handle from the list being polled for.
     */
    virtual void Remove(Pollable*) = 0;

    /** Adds a new timer. Timers can be single-shot or repeating.
     *
     * @param first    The time at which the callback should happen, or 0 for
     *                 straightaway.
     * @param repeatms The timer repeat rate in milliseconds, or 0 for single
     *                 shot.
     * @param callback The callback.
     */
    virtual void Add(time_t first, unsigned int repeatms,
			  Timed *callback) = 0;
    virtual void Remove(Timed*) = 0;

    /** Wake the poller up, e.g. from another thread, whatever it's doing.
     */
    virtual void Wake() = 0;
};

/** Implementation of PollerInterface suitable for the main loop of a thread.
 *
 * For instance, the main loop of the choraled daemon consists only of repeated
 * calls to Poller::Poll().
 */
class Poller: public PollerInterface
{
public:
    class Impl;

private:
    Impl *m_impl;
    
public:
    Poller();
    ~Poller();

    enum { INFINITE_MS = (unsigned int)-1 };

    unsigned Poll(unsigned int timeout_ms);

    // Being a PollerInterface
    void Add(Pollable*, const Callback& callback, 
	     unsigned int direction);
    void Remove(Pollable*);

    void Add(time_t first, unsigned int repeatms, Timed*);
    void Remove(Timed*);

    void Wake();
};

} // namespace util

#endif
