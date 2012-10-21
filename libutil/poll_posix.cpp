#include "config.h"
#include "poll.h"
#include "trace.h"

#if defined(HAVE_POLL_H) && defined(HAVE_SYS_POLL_H)

#include <poll.h>
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

namespace util {

namespace posix {

PollerCore::PollerCore()
    : m_array(NULL),
      m_count(0)
{
}

unsigned int PollerCore::SetUpArray(const std::map<int,Pollable*> *pollables,
				    const std::map<int,unsigned int> *dirs)
{
    struct pollfd *newarray = (struct pollfd*)realloc(m_array,
						      pollables->size() * sizeof(struct pollfd));
    if (!newarray)
	return ENOMEM;
    m_array = newarray;
    
    size_t i = 0;
    for (std::map<int,Pollable*>::const_iterator j = pollables->begin();
	 j != pollables->end();
	 ++j)
    {
	m_array[i].fd = j->first;
	short events = 0;
	unsigned int directions = 0;
	std::map<int,unsigned int>::const_iterator k = dirs->find(j->first);
	if (k != dirs->end())
	    directions = k->second;

	if (directions & PollerInterface::IN)
	    events |= POLLIN;
	if (directions & PollerInterface::OUT)
	    events |= POLLOUT;
//		TRACE << "Waiting on fd " << j->first << " events "
//		      << events << "\n";
	m_array[i].events = events;
	++i;
    }

    m_count = i;

    return 0;
}

unsigned int PollerCore::Poll(unsigned int timeout_ms)
{
    int rc = ::poll(m_array, m_count, (int)timeout_ms);
    if (rc < 0)
	return errno;
    return 0;
}

unsigned int PollerCore::DoCallbacks(const std::map<int,Pollable*> *pollables)
{
    for (size_t i=0; i<m_count; ++i)
    {
	if (m_array[i].revents)
	{
//	    TRACE << "Activity on fd " << m_array[i].fd << "\n";
	    std::map<int,Pollable*>::const_iterator j
		= pollables->find(m_array[i].fd);
	    if (j != pollables->end())
	    {
		unsigned int rc = j->second->OnActivity();
		if (rc != 0)
		    return rc;
	    }
	}
    }
    return 0;
}

PollerCore::~PollerCore()
{
    free(m_array);
}

PollWaker::PollWaker(PollerInterface *p, Pollable *pollable)
    : m_pollable(pollable)
{
    int rc = pipe(m_fd);
    if (rc != 0)
    {
	TRACE << "Can't pipe\n";
    }
    p->AddHandle(m_fd[0], this, PollerInterface::IN);
}

PollWaker::~PollWaker()
{
    close(m_fd[0]);
    close(m_fd[1]);
}

unsigned PollWaker::OnActivity()
{
    char ch;
    ssize_t rc = read(m_fd[0], &ch, 1);
    assert(rc == 1);
    rc = rc;
    return m_pollable ? m_pollable->OnActivity() : 0;
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

#endif // defined(HAVE_POLL_H) && defined(HAVE_SYS_POLL_H)
