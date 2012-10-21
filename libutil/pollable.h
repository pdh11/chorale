#ifndef LIBUTIL_POLLABLE_H
#define LIBUTIL_POLLABLE_H 1

namespace util {

namespace posix {

typedef int   PollHandle; /* File descriptor */
const PollHandle NOT_POLLABLE = -1;

} // namespace posix

namespace win32 {

typedef void* PollHandle; /* HANDLE */
const PollHandle NOT_POLLABLE = 0;

} // namespace win32

#ifdef WIN32
namespace pollapi = ::util::win32;
#else
namespace pollapi = ::util::posix;
#endif

using pollapi::PollHandle;
using pollapi::NOT_POLLABLE;

/** Something which can be waited-for using a PollHandle.
 *
 */
class Pollable
{
public:
    virtual ~Pollable() {}
    virtual PollHandle GetReadHandle() { return NOT_POLLABLE; }
    virtual PollHandle GetWriteHandle() { return NOT_POLLABLE; }
};

} // namespace util

#endif
