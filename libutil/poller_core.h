#ifndef LIBUTIL_POLLER_CORE_H
#define LIBUTIL_POLLER_CORE_H 1

#include <vector>
#include "bind.h"
#include "poll.h"

/* Implementation details and platform-independence support for Poller */

namespace util {

struct PollRecord
{
    Pollable *p;
    Callback c;
    util::PollHandle h;
    unsigned int direction;
};

namespace posix {

/** For explicitly waking-up a Poller
 */
class PollWaker: public Pollable
{
    util::PollHandle m_fd[2];

    unsigned OnActivity();

    util::PollHandle GetReadHandle() { return m_fd[0]; }

public:
    /** Construct a PollWaker for waking up the given PollerInterface. 
     */
    PollWaker(PollerInterface*);
    ~PollWaker();
    
    void Wake();
};

class PollerCore
{
    pollfd *m_array;
    size_t m_count;

public:
    PollerCore();
    ~PollerCore();

    unsigned int SetUpArray(const std::vector<PollRecord>*);
    unsigned int Poll(unsigned int timeout_ms);
    unsigned int DoCallbacks(const std::vector<PollRecord>*, bool valid);
};

} // namespace util::posix

namespace win32 {

/** For explicitly waking-up a Poller
 */
class PollWaker: public Pollable
{
    util::PollHandle m_event;

    unsigned OnActivity();

public:
    /** Construct a PollWaker for waking up the given PollerInterface. 
     */
    PollWaker(PollerInterface*);
    ~PollWaker();
    
    void Wake();

    util::PollHandle GetReadHandle() { return m_event; }
};

class PollerCore
{
    void **m_array;
    size_t m_count;
    int m_which;
    void *m_which_handle;

public:
    PollerCore();
    ~PollerCore();

    unsigned int SetUpArray(const std::vector<PollRecord>*);
    unsigned int Poll(unsigned int timeout_ms);
    unsigned int DoCallbacks(const std::vector<PollRecord>*, bool valid);
};

} // namespace util::win32

} // namespace util

#endif
