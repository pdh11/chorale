#ifndef LIBUTIL_POLLABLE_H
#define LIBUTIL_POLLABLE_H 1

namespace util {

typedef int PollHandle; /* File descriptor (Posix), SOCKET (Win32) */
const PollHandle NOT_POLLABLE = -1;

/** Something which can be waited-for using a PollHandle.
 *
 */
class Pollable
{
public:
    virtual ~Pollable() {}

    virtual PollHandle GetHandle() { return NOT_POLLABLE; }
};

} // namespace util

#endif
