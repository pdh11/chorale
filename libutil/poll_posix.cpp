#include "config.h"
#include "poll.h"
#include "poller_core.h"
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

namespace util {

namespace posix {

PollerCore::PollerCore()
    : m_array(NULL),
      m_count(0)
{
}

unsigned int PollerCore::SetUpArray(const std::vector<PollRecord> *pollables)
{
    m_count = pollables->size();
    if (!m_count)
	return 0;

    struct pollfd *newarray = 
	(struct pollfd*)realloc(m_array, m_count*sizeof(struct pollfd));

    if (!newarray)
	return ENOMEM;

    m_array = newarray;

    for (size_t i=0; i<m_count; ++i)
    {
	m_array[i].fd = (*pollables)[i].h;
	short events = 0;
	unsigned int directions = (*pollables)[i].direction;

	if (directions & PollerInterface::IN)
	    events |= POLLIN;
	if (directions & PollerInterface::OUT)
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
//    TRACE << "pc" << this << " polling\n"; 
    int rc = ::poll(m_array, m_count, (int)timeout_ms);
//    TRACE << "pc" << this << " polled " << rc << "\n";
    if (rc < 0)
	return errno;
    return 0;
}

unsigned int PollerCore::DoCallbacks(const std::vector<PollRecord> *pollables,
				     bool array_valid)
{
    if (!array_valid)
	return 0; // All Posix pollables are level-triggered

    assert(pollables->size() == m_count);
 
//    for (size_t i=0; i<m_count; ++i)
//    {
//	TRACE << i << ". arrayfd=" << m_array[i].fd << " pollables[i].h="
//	      << (*pollables)[i].h << " p=" << (*pollables)[i].p << "\n";
//    }

    /* Care here as "pollables" might get changed as soon as we start making
     * callbacks.
     */
    std::vector<Callback> callbacks;
    callbacks.reserve(pollables->size());

    for (size_t i=0; i<m_count; ++i)
    {
	if (m_array[i].revents)
	{
//	    TRACE << "pc" << this << ": activity on fd " << m_array[i].fd << "\n";
	    assert((*pollables)[i].h == m_array[i].fd);
	    callbacks.push_back((*pollables)[i].c);
	}
    }

    for (size_t i=0; i<callbacks.size(); ++i)
	callbacks[i]();

    return 0;
}

PollerCore::~PollerCore()
{
    free(m_array);
}

PollWaker::PollWaker(PollerInterface *p)
{
    int rc = pipe(m_fd);
    if (rc != 0)
    {
	TRACE << "Can't pipe\n";
    }
    int flags = fcntl(m_fd[0], F_GETFL);
    fcntl(m_fd[0], F_SETFL, flags | O_NONBLOCK);

    p->Add(this, Bind<PollWaker, &PollWaker::OnActivity>(this), 
	   PollerInterface::IN);
}

PollWaker::~PollWaker()
{
    close(m_fd[0]);
    close(m_fd[1]);
}

/** This is level-triggered, we don't want to be woken N times after N calls
 * to Wake().
 */
unsigned PollWaker::OnActivity()
{
    enum { COUNT = 64 };
    char ch[COUNT];
    for (;;) 
    {
	ssize_t rc = read(m_fd[0], ch, COUNT);
	if (rc < 0)
	    return 0;
    }
    return 0;
}

void PollWaker::Wake()
{
    char ch = '*';
    ssize_t rc = write(m_fd[1], &ch, 1);
    assert(rc == 1);
    rc = rc;
}

} // namespace util::posix

} // namespace util

#endif // HAVE_POLL_H && HAVE_SYS_POLL_H
