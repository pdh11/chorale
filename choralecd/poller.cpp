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
#include <QApplication>
#include <sys/time.h>
#include <time.h>

namespace choraleqt {

Notifier::Notifier(int fd, util::Pollable* p, unsigned int direction)
    : QSocketNotifier(fd, (direction==util::PollerInterface::IN) ? Read : Write),
      m_pollable(p)
{
    connect(this, SIGNAL(activated(int)), this, SLOT(OnActivity(int)));
}

void Notifier::OnActivity(int fd)
{
//    TRACE << "Notifier(" << fd << ")::OnActivity\n";
    m_pollable->OnActivity();
}

Timer::Timer(unsigned int repeatms, util::Timed *timed)
    : QTimer(),
      m_timed(timed),
      m_repeatms(repeatms)
{
    connect(this, SIGNAL(timeout()), this, SLOT(OnTimer()));
    setSingleShot(true);
}

void Timer::OnTimer()
{
    if (m_repeatms)
    {
	setSingleShot(false);
	start(m_repeatms);
	m_repeatms = 0;
    }
    m_timed->OnTimer();
}


Poller::Poller()
{
//    TRACE << "Poller is on " << pthread_self() << "\n";
}

Poller::~Poller()
{
    // @todo Delete everything
}

void Poller::AddHandle(int fd, util::Pollable *callback, 
		       unsigned int direction)
{
    CheckThread();
//    TRACE << "Creating notifier on " << pthread_self() << "\n";
    m_map[fd] = new Notifier(fd, callback, direction);
}

void Poller::RemoveHandle(int poll_handle)
{
    CheckThread();
    delete m_map[poll_handle];
    m_map.erase(poll_handle);
}

void Poller::AddTimer(time_t first, unsigned int repeatms, 
		      util::Timed *callback)
{
    timeval now;
    ::gettimeofday(&now, NULL);
    uint64_t nowms = now.tv_sec*(uint64_t)1000 + now.tv_usec/1000;

    uint64_t firstms = first*(uint64_t)1000;

    Timer* t = new Timer(repeatms, callback);
    t->moveToThread(QApplication::instance()->thread());
    boost::mutex::scoped_lock lock(m_mutex);
    m_timermap[callback] = t;
    t->start((firstms > nowms) ? (int)(firstms - nowms) : 0);
}

void Poller::RemoveTimer(util::Timed *callback)
{
    delete m_timermap[callback];
    boost::mutex::scoped_lock lock(m_mutex);
    m_timermap.erase(callback);
}

} // namespace choraleqt
