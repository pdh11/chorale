#include "config.h"
#include "poll.h"
#include "trace.h"
#include "bind.h"

#if HAVE_POLL_H && HAVE_SYS_POLL_H

#include <poll.h>
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>

LOG_DECL(POLL);

namespace util {

namespace posix {

PollerCore::PollerCore()
    : m_array(NULL),
      m_count(0)
{
    int rc = pipe(m_waker_fd);
    if (rc != 0)
    {
	TRACE << "Can't pipe\n";
    }
    int flags = fcntl(m_waker_fd[0], F_GETFL);
    fcntl(m_waker_fd[0], F_SETFL, flags | O_NONBLOCK);
}

unsigned int PollerCore::SetUpArray(const std::vector<PollRecord> *pollables)
{
    m_count = pollables->size() + 1;

    struct pollfd *newarray = 
	(struct pollfd*)realloc(m_array, m_count*sizeof(struct pollfd));

    if (!newarray)
	return ENOMEM;

    m_array = newarray;

    m_array[0].fd = m_waker_fd[0];
    m_array[0].events = POLLIN;
    m_array[0].revents = 0;

    for (size_t i=1; i<m_count; ++i)
    {
	m_array[i].fd = (*pollables)[i-1].h;
	short events = 0;
	unsigned int directions = (*pollables)[i-1].direction;

	if (directions & PollRecord::IN)
	    events |= POLLIN;
	if (directions & PollRecord::OUT)
	    events |= POLLOUT;
//	TRACE << "pc" << this << ": " << i << ". fd " << m_array[i].fd << " events "
//	      << events << " pollable " << (*pollables)[i].p << "\n";
	m_array[i].events = events;
	m_array[i].revents = 0;
    }

    return 0;
}

unsigned int PollerCore::Poll(unsigned int timeout_ms)
{
//    TRACE << "pc" << this << " polling (" << timeout_ms << "ms)\n"; 
    int rc = ::poll(m_array, m_count, (int)timeout_ms);
//    TRACE << "pc" << this << " polled " << rc << "\n";
    if (rc < 0 && errno != EINTR)
	return errno;
    return 0;
}

unsigned int PollerCore::DoCallbacks(std::vector<PollRecord> *pollables,
				     bool array_valid)
{
    if (!array_valid)
	return 0; // All Posix pollables are level-triggered

    assert(pollables->size() + 1 == m_count);

    if (m_array[0].revents)
    {
	LOG(POLL) << "Got wake\n";

	// Wipe up wake-up events
	enum { COUNT = 16 };
	char ch[COUNT];
	for (;;) 
	{
	    ssize_t rc = read(m_waker_fd[0], ch, COUNT);
	    if (rc <= 0)
		break;
	}
    }
 
//    for (size_t i=1; i<m_count; ++i)
//    {
//	TRACE << i << ". arrayfd=" << m_array[i].fd << " pollables[i-1].h="
//	      << (*pollables)[i-1].h << " p=" << (*pollables)[i-1].p << "\n";
//    }

    /* Care here as "pollables" might get changed as soon as we start making
     * callbacks.
     */
    std::vector<TaskCallback> task_callbacks;
    task_callbacks.reserve(pollables->size());

    for (size_t i=1; i<m_count; ++i)
    {
	if (m_array[i].revents)
	{
//	    TRACE << "pc" << this << ": activity on fd " << m_array[i].fd << "\n";
	    PollRecord& record = (*pollables)[i-1];
	    assert(record.h == m_array[i].fd);

	    if (record.oneshot)
		record.direction = 0; // Mark for deletion

	    if (record.tc)
		task_callbacks.push_back(record.tc);
	}
    }

//    TRACE << "Making " << task_callbacks.size() << " callbacks\n";

    for (size_t i=0; i<task_callbacks.size(); ++i)
	task_callbacks[i]();

//    TRACE << "Done\n";

    return 0;
}

void PollerCore::Wake()
{
//    TRACE << "Wake writes\n";
    char ch = '*';
    ssize_t rc = write(m_waker_fd[1], &ch, 1);
    assert(rc == 1);
    rc = rc;
}

PollerCore::~PollerCore()
{
    free(m_array);
    close(m_waker_fd[0]);
    close(m_waker_fd[1]);
}

} // namespace util::posix

} // namespace util

#endif // HAVE_POLL_H && HAVE_SYS_POLL_H
