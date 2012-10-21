#ifndef LIBUTIL_POLLER_CORE_H
#define LIBUTIL_POLLER_CORE_H 1

#include <vector>
#include "bind.h"
#include "pollable.h"
#include "task.h"

struct pollfd;

/* Implementation details and platform-independence support for Poller */

namespace util {

struct PollRecord
{
    TaskCallback tc;
    void *internal;
    util::PollHandle h;
    unsigned char direction;
    enum { IN = 1, OUT = 2 };
    bool oneshot;
};

namespace posix {

class PollerCore
{
    pollfd *m_array;
    size_t m_count;

    util::PollHandle m_waker_fd[2];

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

namespace win32 {

class PollerCore
{
    void **m_array;
    size_t m_count;
    int m_which;
    void *m_which_handle;
    void *m_wakeup_event;

public:
    PollerCore();
    ~PollerCore();

    unsigned int SetUpArray(std::vector<PollRecord>*);
    unsigned int Poll(unsigned int timeout_ms);
    unsigned int DoCallbacks(std::vector<PollRecord>*, bool valid);
    void Deleting(PollRecord*);
    void Wake();
};

} // namespace util::win32

#ifdef WIN32
namespace pollapi = ::util::win32;
#else
namespace pollapi = ::util::posix;
#endif

} // namespace util

#endif
