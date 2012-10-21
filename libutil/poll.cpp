#include "poll.h"
#include <map>
#include <boost/thread/mutex.hpp>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include "trace.h"

namespace util {

class Poller::Impl
{
    typedef std::map<int, Pollable*> handles_t;
    typedef std::map<int, unsigned int> directions_t;

    boost::mutex m_mutex;    
    handles_t m_handles;
    directions_t m_directions;

    struct pollfd *m_array;
    bool m_array_valid;

public:
    Impl();
    ~Impl();

    void AddHandle(int, Pollable*, unsigned int);
    void RemoveHandle(int);

    unsigned Poll(unsigned int timeout_ms);
};

Poller::Impl::Impl()
    : m_array(NULL),
      m_array_valid(false)
{
}

Poller::Impl::~Impl()
{
    free(m_array);
}

void Poller::Impl::AddHandle(int fd, Pollable *callback,
			     unsigned int direction)
{
    boost::mutex::scoped_lock lock(m_mutex);

    m_directions[fd] = direction;
    m_handles[fd] = callback;
    m_array_valid = false;
}

void Poller::Impl::RemoveHandle(int fd)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_directions.erase(fd);
    m_handles.erase(fd);
    m_array_valid = false;
}

unsigned Poller::Impl::Poll(unsigned int timeout_ms)
{
    nfds_t nfds;

    {
	boost::mutex::scoped_lock lock(m_mutex);

	if (!m_array_valid)
	{
	    struct pollfd *newarray = (struct pollfd*) realloc(m_array, 
					      m_handles.size() * sizeof(struct pollfd));
	    if (!newarray)
		return ENOMEM;
	    m_array = newarray;
	    size_t i = 0;
	    for (handles_t::const_iterator j = m_handles.begin();
		 j != m_handles.end();
		 ++j)
	    {
		m_array[i].fd = j->first;
		int events = 0;
		unsigned int directions = m_directions[j->first];
		if (directions & IN)
		    events |= POLLIN;
		if (directions & OUT)
		    events |= POLLOUT;
//		TRACE << "Waiting on fd " << j->first << " events "
//		      << events << "\n";
		m_array[i].events = events;
		++i;
	    }
	    nfds = (nfds_t)i;
	    m_array_valid = true;
	}
	else
	{
	    nfds = m_handles.size();
//	    TRACE << "Re-waiting on " << nfds << " fds\n";
	}
    }

    int rc = ::poll(m_array, nfds, timeout_ms);

//    TRACE << "polled " << rc << "\n";

    if (rc < 0)
	return errno;
    if (rc == 0)
	return 0;

    for (nfds_t i=0; i<nfds; ++i)
    {
	if (m_array[i].revents)
	{
//	    TRACE << "Activity on fd " << m_array[i].fd << "\n";
	    m_handles[m_array[i].fd]->OnActivity();
	}
    }
//    TRACE << "Done activity calls\n";

    return 0;
}

Poller::Poller()
    : m_impl(new Impl)
{
}

Poller::~Poller()
{
    delete m_impl;
}

void Poller::AddHandle(int fd, Pollable *p, unsigned int direction)
{
    m_impl->AddHandle(fd, p, direction);
}

void Poller::RemoveHandle(int fd)
{
    m_impl->RemoveHandle(fd);
}

unsigned Poller::Poll(unsigned int timeout_ms)
{
    return m_impl->Poll(timeout_ms);
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
    read(m_fd[0], &ch, 1);
    return m_pollable ? m_pollable->OnActivity() : 0;
}

void PollWaker::Wake()
{
    char ch = '*';
    write(m_fd[1], &ch, 1);
}

}; // namespace util
