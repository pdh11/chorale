#ifndef LIBUTIL_LOCKING_H
#define LIBUTIL_LOCKING_H 1

#include "mutex.h"

namespace util {
    
/** Locking policy: per-object
 */
class PerObjectLocking
{
    Mutex m_mutex;

public:
    class Lock
    {
	Mutex::Lock m_lock;
    public:
	Lock(PerObjectLocking *l);
	~Lock();
    };

    PerObjectLocking();
    ~PerObjectLocking();
};
    
/** Locking policy: per-object, recursive
 */
class PerObjectRecursiveLocking
{
    RecursiveMutex m_mutex;

public:
    class Lock
    {
	RecursiveMutex::Lock m_lock;

    public:
	Lock(PerObjectRecursiveLocking *l);
	~Lock();
    };

    PerObjectRecursiveLocking();
    ~PerObjectRecursiveLocking();
};

/** Locking policy: per-class
 */
template <class T>
class PerClassLocking
{
    static Mutex sm_mutex;

public:
    class Lock
    {
	Mutex::Lock m_lock;

    public:
	Lock(PerClassLocking<T> *l) : m_lock(l->sm_mutex) {}
    };
};

template<class T>
Mutex util::PerClassLocking<T>::sm_mutex;

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
