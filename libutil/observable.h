#ifndef LIBUTIL_OBSERVABLE_H
#define LIBUTIL_OBSERVABLE_H 1

#include <boost/thread/mutex.hpp>
#include <list>

namespace util {

template <class Observer>
class Observable
{
    boost::mutex m_mutex;
    typedef std::list<Observer*> observers_t;
    observers_t m_observers;

public:
    
    void AddObserver(Observer *obs)
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_observers.push_back(obs);
    }

    void RemoveObserver(Observer *obs)
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_observers.remove(obs);
    }

    template <typename T1, typename T2>
    void Fire(void (Observer::*fn)(T1, T2), T1 t1, T2 t2)
    {
	boost::mutex::scoped_lock lock(m_mutex);
	for (typename observers_t::const_iterator i = m_observers.begin();
	     i != m_observers.end();
	     ++i)
	    ((*i)->*fn)(t1, t2);
    }

    template <typename T>
    void Fire(void (Observer::*fn)(T), T t)
    {
	boost::mutex::scoped_lock lock(m_mutex);
	for (typename observers_t::const_iterator i = m_observers.begin();
	     i != m_observers.end();
	     ++i)
	    ((*i)->*fn)(t);
    }

    template <typename T>
    void Fire(void (Observer::*fn)(const T&), T t)
    {
	boost::mutex::scoped_lock lock(m_mutex);
	for (typename observers_t::const_iterator i = m_observers.begin();
	     i != m_observers.end();
	     ++i)
	    ((*i)->*fn)(t);
    }

    void Fire(void (Observer::*fn)(void))
    {
	boost::mutex::scoped_lock lock(m_mutex);
	for (typename observers_t::const_iterator i = m_observers.begin();
	     i != m_observers.end();
	     ++i)
	    ((*i)->*fn)();
    }
};

}; // namespace util

#endif
