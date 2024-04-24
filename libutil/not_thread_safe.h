#ifndef LIBUTIL_NOT_THREAD_SAFE_H
#define LIBUTIL_NOT_THREAD_SAFE_H 1

#include "config.h"
#include <pthread.h>
#include <assert.h>

namespace util {

#if DEBUG
namespace posix {

class NotThreadSafe
{
    pthread_t m_thread;
public:
    NotThreadSafe() : m_thread(pthread_self()) {}

    void CheckThread() { assert(m_thread == pthread_self()); }
};

} // namespace pthread
#endif // DEBUG

/** Portability classes, %empty stub versions (cf util::win32, util::posix)
 */
namespace empty {

struct NotThreadSafe
{
    static void CheckThread() {}
};

} // namespace empty

#if DEBUG
namespace threadapi = posix;
#else
namespace threadapi = empty;
#endif

using threadapi::NotThreadSafe;

} // namespace util

#endif
