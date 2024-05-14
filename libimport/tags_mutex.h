#ifndef TAGS_MUTEX_H
#define TAGS_MUTEX_H 1

#include <mutex>

namespace import {

/** This shouldn't be necessary, but TagLib's string operations aren't
 * thread-safe.
 */
extern std::mutex s_taglib_mutex;

} // namespace import

#endif
