#ifndef LIBUTIL_LOCKING_H
#define LIBUTIL_LOCKING_H 1

#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace util {
    
/** Locking policy: per-object
 */
class PerObjectLocking
{
    boost::mutex m_mutex;

public:
    class Lock
    {
	boost::mutex::scoped_lock m_lock;
    public:
	Lock(PerObjectLocking *l) : m_lock(l->m_mutex) {}
    };
};
    
/** Locking policy: per-object, recursive
 */
class PerObjectRecursiveLocking
{
    boost::recursive_mutex m_mutex;

public:
    class Lock
    {
	boost::recursive_mutex::scoped_lock m_lock;

    public:
	Lock(PerObjectRecursiveLocking *l) : m_lock(l->m_mutex) {}
    };
};

/** Locking policy: per-class
 */
template <class T>
class PerClassLocking
{
    static boost::mutex sm_mutex;

public:
    class Lock
    {
	boost::mutex::scoped_lock m_lock;

    public:
	Lock(PerClassLocking<T> *l) : m_lock(l->sm_mutex) {}
    };
};

template<class T>
boost::mutex util::PerClassLocking<T>::sm_mutex;

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

} // namespace util

#endif
