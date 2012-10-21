#ifndef LIBUTIL_POLL_H
#define LIBUTIL_POLL_H 1

#include <string.h>

namespace util {

class Pollable
{
public:
    virtual ~Pollable() {}

    virtual unsigned OnActivity() = 0;
};

/** Abstract poller interface, implemented by util::PollThread and
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
};

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
};

/** For explicitly waking-up a Poller
 */
class PollWaker: private Pollable
{
    int m_fd[2];
    Pollable *m_pollable;

    unsigned OnActivity();

public:
    /** Construct a PollWaker for waking up the given PollerInterface. If the
     * given Pollable is non-NULL, its OnActivity is called at wake time too.
     */
    PollWaker(PollerInterface*, Pollable* = NULL);
    ~PollWaker();
    
    void Wake();
};

};

#endif
