#ifndef LIBUTIL_OBSERVABLE_H
#define LIBUTIL_OBSERVABLE_H 1

#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <list>
#include <functional>
#include <assert.h>

namespace util {

/** Storage policy: one observer
 */
template <class Observer>
class OneObserver
{
    Observer *m_observer;

public:
    OneObserver() : m_observer(NULL) {}

protected:
    void AddObserver(Observer *obs) { assert(!m_observer); m_observer = obs; }
    void RemoveObserver(Observer *obs)
    { 
	if (obs)
	{
	    assert(m_observer == obs);
	    m_observer = NULL;
	}
    }

    template <typename F>
    void Apply(F f) { if (m_observer) f(m_observer); }
};

/** Storage policy: list of observers
 */
template <class Observer>
class ObserverList
{
    typedef std::list<Observer*> observers_t;
    observers_t m_observers;

public:
    ObserverList() {}
    
protected:
    void AddObserver(Observer *obs)
    {
	m_observers.push_back(obs);
    }

    void RemoveObserver(Observer *obs)
    {
	m_observers.remove(obs);
    }

    template <typename F>
    void Apply(F f)
    {
	for (typename observers_t::const_iterator i = m_observers.begin();
	     i != m_observers.end();
	     ++i)
	    f(*i);
    }
};

/** Locking policy: per-object
 */
class PerObjectLocking
{
    boost::mutex m_mutex;

public:
    class Lock: public boost::mutex::scoped_lock
    {
    public:
	Lock(PerObjectLocking *l): boost::mutex::scoped_lock(l->m_mutex) {}
    };
};

/** Locking policy: per-class
 */
class PerClassLocking
{
    static boost::mutex sm_mutex;

public:
    class Lock: public boost::mutex::scoped_lock
    {
    public:
	Lock(PerClassLocking *l): boost::mutex::scoped_lock(l->sm_mutex) {}
    };
};

/** Locking policy: none
 */
class NoLocking
{
public:
    class Lock
    {
    public:
	Lock(NoLocking*) {}
    };
};

/** Base class that encapsulates telling observer classes what's going on.
 *
 * To use this class, inherit from it (choosing a StoragePolicy -- one
 * observer or several -- and a LockingPolicy) and issue
 * "Fire(&YourObserver::OnThingy, args)" as needed.
 *
 * You only need a non-empty LockingPolicy if your list of observers might be
 * changed while a call is happening.
 */
template <class Observer, class StoragePolicy=ObserverList<Observer>,
	  class LockingPolicy=PerObjectLocking>
class Observable: public StoragePolicy, public LockingPolicy
{
public:
    void AddObserver(Observer *obs)
    {
	typename LockingPolicy::Lock(this);
	StoragePolicy::AddObserver(obs);
    }

    void RemoveObserver(Observer *obs)
    {
	typename LockingPolicy::Lock(this);
	StoragePolicy::RemoveObserver(obs);
    }

    template <typename T1, typename T2>
    void Fire(void (Observer::*fn)(T1, T2), T1 t1, T2 t2)
    {
	typename LockingPolicy::Lock(this);
	StoragePolicy::Apply(boost::bind(fn, _1, t1, t2));
    }

    template <typename T1, typename T2>
    void Fire(void (Observer::*fn)(T1, const T2&), T1 t1, const T2& t2)
    {
	typename LockingPolicy::Lock(this);
	StoragePolicy::Apply(boost::bind(fn, _1, t1, boost::cref(t2)));
    }

    template <typename T>
    void Fire(void (Observer::*fn)(T), T t)
    {
	typename LockingPolicy::Lock(this);
	StoragePolicy::Apply(boost::bind(fn, _1, t));
    }

    template <typename T>
    void Fire(void (Observer::*fn)(const T&), const T& t)
    {
	typename LockingPolicy::Lock(this);
	StoragePolicy::Apply(boost::bind(fn, _1, boost::cref(t)));
    }

    void Fire(void (Observer::*fn)(void))
    {
	typename LockingPolicy::Lock(this);
	StoragePolicy::Apply(boost::mem_fn(fn));
    }
};

} // namespace util

#endif
