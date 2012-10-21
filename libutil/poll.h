#ifndef LIBUTIL_POLL_H
#define LIBUTIL_POLL_H 1

#include <string.h>
#include <time.h>
#include <map>

#undef IN
#undef OUT

struct pollfd;

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

    /** Add a handle to the list being polled for.
     *
     * On Posix systems, poll_handle is a file descriptor, and
     * direction should be set appropriately. On Win32, poll_handle is
     * a HANDLE, and direction is ignored. This API does not support
     * Win64 (where sizeof(HANDLE) != sizeof(int)).
     */
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
    void AddHandle(int poll_handle, Pollable *callback, 
		   unsigned int direction);
    void RemoveHandle(int poll_handle);

    void AddTimer(time_t first, unsigned int repeatms, Timed*);
    void RemoveTimer(Timed*);
};

namespace posix {

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

class PollerCore
{
    pollfd *m_array;
    size_t m_count;

public:
    PollerCore();
    ~PollerCore();

    unsigned int SetUpArray(const std::map<int,Pollable*>*,
			    const std::map<int,unsigned int>*);
    unsigned int Poll(unsigned int timeout_ms);
    unsigned int DoCallbacks(const std::map<int,Pollable*>*);
};

} // namespace util::posix

namespace win32 {

/** For explicitly waking-up a Poller
 */
class PollWaker: private Pollable
{
    Pollable *m_pollable;
    void *m_event;

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

class PollerCore
{
    void **m_array;
    size_t m_count;
    int m_which;

public:
    PollerCore();
    ~PollerCore();

    unsigned int SetUpArray(const std::map<int,Pollable*>*,
			    const std::map<int,unsigned int>*);
    unsigned int Poll(unsigned int timeout_ms);
    unsigned int DoCallbacks(const std::map<int,Pollable*>*);
};

} // namespace util::win32

#ifdef WIN32
namespace pollapi = ::util::win32;
#else
namespace pollapi = ::util::posix;
#endif

using pollapi::PollWaker;

} // namespace util

#endif
