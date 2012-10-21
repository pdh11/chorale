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
    class Lock: public boost::mutex::scoped_lock
    {
    public:
	Lock(PerObjectLocking *l): boost::mutex::scoped_lock(l->m_mutex) {}
    };
};
    
/** Locking policy: per-object, recursive
 */
class PerObjectRecursiveLocking
{
    boost::recursive_mutex m_mutex;

public:
    class Lock: public boost::recursive_mutex::scoped_lock
    {
    public:
	Lock(PerObjectRecursiveLocking *l)
	    : boost::recursive_mutex::scoped_lock(l->m_mutex)
	{}
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

} // namespace util

#endif
