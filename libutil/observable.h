#ifndef LIBUTIL_OBSERVABLE_H
#define LIBUTIL_OBSERVABLE_H 1

#include <list>
#include <assert.h>
#include "locking.h"

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

    class Iterator
    {
	Observer *m_observer;
    public:
	explicit Iterator(Observer *observer) : m_observer(observer) {}

	Observer *operator*() { return m_observer; }
	void operator++() { m_observer = NULL; }

	bool operator==(const Iterator& other)
	{
	    return !m_observer && !other.m_observer; 
	}

	bool operator!=(const Iterator& other)
	{
	    return m_observer || other.m_observer; 
	}
    };

    typedef Iterator const_iterator;

    const_iterator begin() const { return Iterator(m_observer); }
    const_iterator end() const { return Iterator(NULL); }
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

    typedef typename observers_t::const_iterator const_iterator;

    const_iterator begin() const { return m_observers.begin(); }
    const_iterator end() const { return m_observers.end(); }
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
template <class Observer, template<class Obs> class StoragePolicy=ObserverList,
	  class LockingPolicy=PerObjectLocking>
class Observable: private StoragePolicy<Observer>, private LockingPolicy
{
    typedef StoragePolicy<Observer> Storage;
    typedef typename Storage::const_iterator const_iterator;
    typedef typename LockingPolicy::Lock Lock;
    
    const_iterator begin() { return Storage::begin(); }
    const_iterator end() { return Storage::end(); }

public:
    void AddObserver(Observer *obs)
    {
	Lock lock(this);
	Storage::AddObserver(obs);
    }

    void RemoveObserver(Observer *obs)
    {
	Lock lock(this);
	Storage::RemoveObserver(obs);
    }

    template <typename T1, typename T2, typename T3, typename T4>
    void Fire(void (Observer::*fn)(T1, T2, T3, T4), T1 t1, T2 t2, T3 t3, T4 t4)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	    ((*i)->*fn)(t1, t2, t3, t4);
    }

    template <typename T1, typename T2, typename T3>
    void Fire(void (Observer::*fn)(T1, T2, T3), T1 t1, T2 t2, T3 t3)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	    ((*i)->*fn)(t1, t2, t3);
    }

    template <typename T1, typename T2>
    void Fire(void (Observer::*fn)(T1, T2), T1 t1, T2 t2)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	    ((*i)->*fn)(t1, t2);
    }

    template <typename T1, typename T2>
    void Fire(void (Observer::*fn)(T1, const T2&), T1 t1, const T2& t2)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	    ((*i)->*fn)(t1, t2);
    }

    template <typename T>
    void Fire(void (Observer::*fn)(T), T t)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	    ((*i)->*fn)(t);
    }

    template <typename T>
    unsigned int Fire(unsigned int (Observer::*fn)(T), T t)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	{
	    unsigned int rc = ((*i)->*fn)(t);
	    if (rc)
		return rc;
	}
	return 0;
    }

    template <typename T>
    void Fire(void (Observer::*fn)(const T&), const T& t)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	    ((*i)->*fn)(t);
    }

    template <typename T>
    unsigned int Fire(unsigned int (Observer::*fn)(const T&), const T& t)
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	{
	    unsigned int rc = ((*i)->*fn)(t);
	    if (rc)
		return rc;
	}
	return 0;
    }

    void Fire(void (Observer::*fn)(void))
    {
	Lock lock(this);
	for (const_iterator i = begin(); i != end(); ++i)
	    ((*i)->*fn)();
    }
};

} // namespace util

#endif
