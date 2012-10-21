/* poller.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "poller.h"
#include "libutil/trace.h"
#include <qsocketnotifier.h>

namespace choraleqt {

Notifier::Notifier(int fd, util::Pollable* p, unsigned int direction)
    : QSocketNotifier(fd, (direction==util::PollerInterface::IN) ? Read : Write),
      m_pollable(p)
{
    connect(this, SIGNAL(activated(int)), this, SLOT(OnActivity(int)));
}

void Notifier::OnActivity(int fd)
{
    TRACE << "Notifier(" << fd << ")::OnActivity\n";
    m_pollable->OnActivity();
}

Poller::Poller()
{
    TRACE << "Poller is on " << pthread_self() << "\n";
}

Poller::~Poller()
{
    // @todo Delete everything
}

void Poller::AddHandle(int fd, util::Pollable *callback, 
		       unsigned int direction)
{
    CheckThread();
    TRACE << "Creating notifier on " << pthread_self() << "\n";
    m_map[fd] = new Notifier(fd, callback, direction);
}

void Poller::RemoveHandle(int poll_handle)
{
    CheckThread();
    delete m_map[poll_handle];
}

}; // namespace choraleqt
