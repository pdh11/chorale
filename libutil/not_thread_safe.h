#ifndef LIBUTIL_NOT_THREAD_SAFE_H
#define LIBUTIL_NOT_THREAD_SAFE_H 1

#include <pthread.h>
#include <assert.h>

namespace util {

#if defined(DEBUG)
class NotThreadSafe
{
    pthread_t m_thread;
public:
    NotThreadSafe() : m_thread(pthread_self()) {}
    
    void CheckThread() { assert(m_thread == pthread_self()); }
};
#else
struct NotThreadSafe
{
    static void CheckThread() {}
};
#endif

};

#endif
