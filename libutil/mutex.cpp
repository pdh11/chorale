#include "mutex.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include "bind.h"
#include "trace.h"

namespace util {

template<>
struct WrappedType<Mutex>
{
    typedef boost::mutex type;
};

Mutex::Mutex()
{
}

Mutex::~Mutex()
{
}

template<>
struct WrappedType<Mutex::Lock>
{
    typedef boost::mutex::scoped_lock type;
};

Mutex::Lock::Lock(Mutex& mutex)
    : Wrapper<Lock,SIZEOF_BOOST__MUTEX__SCOPED_LOCK>(mutex.Unwrap())
{
}

Mutex::Lock::~Lock()
{
}

template<>
struct WrappedType<RecursiveMutex>
{
    typedef boost::recursive_mutex type;
};

RecursiveMutex::RecursiveMutex()
{
}

RecursiveMutex::~RecursiveMutex()
{
}

template<>
struct WrappedType<RecursiveMutex::Lock>
{
    typedef boost::recursive_mutex::scoped_lock type;
};

RecursiveMutex::Lock::Lock(RecursiveMutex& mutex)
    : Wrapper<Lock,SIZEOF_BOOST__RECURSIVE_MUTEX__SCOPED_LOCK>(mutex.Unwrap())
{
}

RecursiveMutex::Lock::~Lock()
{
}

template<>
struct WrappedType<Condition>
{
    typedef boost::condition type;
};

Condition::Condition()
{
}

Condition::~Condition()
{
}

bool Condition::Wait(Mutex::Lock& lock, unsigned int sec)
{
#if HAVE_CONDITION_TIMED_WAIT_INTERVAL
    return Unwrap().timed_wait(lock.Unwrap(), boost::posix_time::seconds(sec));
#else
    boost::xtime wakeup;
    boost::xtime_get(&wakeup, boost::TIME_UTC);
    wakeup.sec += sec;
    return Unwrap().timed_wait(lock.Unwrap(), wakeup);
#endif
}

void Condition::NotifyAll()
{
    Unwrap().notify_all();
}

void Condition::NotifyOne()
{
    Unwrap().notify_one();
}

template<>
struct WrappedType<Thread>
{
    typedef boost::thread type;
};

Thread::Thread(const Callback& c)
    : Wrapper<Thread, SIZEOF_BOOST__THREAD>(c)
{
}

Thread::~Thread()
{
}

void Thread::Join()
{
    Unwrap().join();
}

} // namespace util

#ifdef TEST

int main()
{
    /* Not much of a test, but it's one of those C++-ey things where
     * if it actually *compiles*, it works.
     */
    util::Mutex mx;
    util::Condition cond;

    {
	util::Mutex::Lock lock(mx);

	cond.Wait(lock, 3);
    }

    return 0;
}

#endif
