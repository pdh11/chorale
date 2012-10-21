/* libutil/perthread.h */
#ifndef LIBUTIL_PERTHREAD_H
#define LIBUTIL_PERTHREAD_H 1

#include <pthread.h>
#include "libutil/trace.h"

namespace util {

template <class T>
inline void Deleter(void *p)
{
    T *t = (T*) p;
//    TRACE << "thr" << pthread_self() << ": in deleter for " << p << "\n";
    delete t;
}

template <class T>
class PerThread
{
    pthread_key_t m_key;

public:
    PerThread()
    {
	pthread_key_create(&m_key, &Deleter<T>);
    }
       
    ~PerThread()
    {
	TRACE << "Calling pthread_key_delete\n";
	pthread_key_delete(m_key);
	TRACE << "pthread_key_delete done\n";
    }
    
    T *Get() const
    {
	return (T*) pthread_getspecific(m_key);
    }

    void Set(T* t)
    {
	pthread_setspecific(m_key, (void*)t);
    }
};

}; // namespace util

#endif
