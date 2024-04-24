#ifndef LIBUTIL_POLLER_CORE_H
#define LIBUTIL_POLLER_CORE_H 1

#include <vector>
#include "bind.h"
#include "task.h"
#include "counted_pointer.h"

#undef IN
#undef OUT

struct pollfd;

/* Implementation details and platform-independence support for Poller */

namespace util {

struct PollRecord
{
    TaskCallback tc;
    void *internal;
    int h;
    unsigned char direction;
    enum { IN = 1, OUT = 2 };
    bool oneshot;

    PollRecord() : internal(NULL), h(0), direction(IN), oneshot(false) {}
};

namespace posix {

class PollerCore
{
    pollfd *m_array;
    size_t m_count;

    int m_waker_fd[2];

public:
    PollerCore();
    ~PollerCore();

    unsigned int SetUpArray(const std::vector<PollRecord>*);
    unsigned int Poll(unsigned int timeout_ms);
    unsigned int DoCallbacks(std::vector<PollRecord>*, bool valid);
    void Deleting(PollRecord*) {}
    void Wake();
};

} // namespace util::posix

namespace pollapi = ::util::posix;

} // namespace util

#endif
