#ifndef LIBUTIL_NOT_THREAD_SAFE_H
#define LIBUTIL_NOT_THREAD_SAFE_H 1

#include "config.h"

#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <assert.h>

namespace util {

#ifndef WIN32
namespace posix {

class NotThreadSafe
{
    pthread_t m_thread;
public:
    NotThreadSafe() : m_thread(pthread_self()) {}
    
    void CheckThread() { assert(m_thread == pthread_self()); }
};

} // namespace pthread
#endif // HAVE_PTHREAD_H

/** Portability classes, %empty stub versions (cf util::win32, util::posix)
 */
namespace empty {

struct NotThreadSafe
{
    static void CheckThread() {}
};

} // namespace empty

#if defined(DEBUG) && !defined(WIN32)
namespace threadapi = posix;
#else
namespace threadapi = empty;
#endif

using threadapi::NotThreadSafe;

} // namespace util

#endif
