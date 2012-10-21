#ifndef LIBUTIL_MUTEX_H
#define LIBUTIL_MUTEX_H 1

#include "config.h"
#include "wrapper.h"

namespace util {

class Callback;

/** Wrap a boost::mutex so we don't include so very many headers.
 */
class Mutex: public Wrapper<Mutex, SIZEOF_BOOST__MUTEX>
{
public:
    Mutex();
    ~Mutex();

    class Lock: public Wrapper<Lock, SIZEOF_BOOST__MUTEX__SCOPED_LOCK>
    {
    public:
	Lock(Mutex&);
	~Lock();
    };
};

class RecursiveMutex: public Wrapper<RecursiveMutex, 
				     SIZEOF_BOOST__RECURSIVE_MUTEX>
{
public:
    RecursiveMutex();
    ~RecursiveMutex();

    /** Wrapper for boost::recursive_mutex::scoped_lock.
     *
     * We could, in fact, just lock/unlock the mutex -- there's no
     * need to wrap the Boost type here. We do so only for symmetry
     * with Mutex::Lock (where we need to wrap the genuine Boost
     * objects so that we can pass them to boost::condition::wait).
     */
    class Lock: public Wrapper<Lock, 
			       SIZEOF_BOOST__RECURSIVE_MUTEX__SCOPED_LOCK>
    {
    public:
	Lock(RecursiveMutex&);
	~Lock();
    };
};

class Condition: public Wrapper<Condition, SIZEOF_BOOST__CONDITION>
{
public:
    Condition();
    ~Condition();

    bool Wait(Mutex::Lock&, unsigned int sec);

    void NotifyOne();
    void NotifyAll();
};

class Thread: public Wrapper<Thread, SIZEOF_BOOST__THREAD>
{
public:
    Thread(const Callback& cb);
    ~Thread();

    void Join();
};

} // namespace util

#endif
