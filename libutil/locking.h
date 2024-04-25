#ifndef LIBUTIL_LOCKING_H
#define LIBUTIL_LOCKING_H 1

#include <mutex>

namespace util {
    
/** Locking policy: per-object
 */
class PerObjectLocking
{
    std::mutex m_mutex;

public:
    class Lock
    {
        std::lock_guard<std::mutex> m_lock;
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
    std::recursive_mutex m_mutex;

public:
    class Lock
    {
        std::lock_guard<std::recursive_mutex> m_lock;

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
    static std::mutex sm_mutex;

public:
    class Lock
    {
        std::lock_guard<std::mutex> m_lock;

    public:
	Lock(PerClassLocking<T> *l) : m_lock(l->sm_mutex) {}
    };
};

template<class T>
std::mutex util::PerClassLocking<T>::sm_mutex;

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
