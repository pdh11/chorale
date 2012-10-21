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
#include "libutil/bind.h"
#include <qsocketnotifier.h>
#include <QApplication>
#include <sys/time.h>
#include <time.h>

namespace choraleqt {

Notifier::Notifier(int fd, const util::Callback& callback,
		   QSocketNotifier::Type direction)
    : QSocketNotifier(fd, direction),
      m_callback(callback)
{
    connect(this, SIGNAL(activated(int)), this, SLOT(OnActivity(int)));
}

void Notifier::OnActivity(int)
{
//    TRACE << "Notifier(" << fd << ")::OnActivity\n";
    m_callback();
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
    /// @todo Delete everything
}

void Poller::Add(util::Pollable *p, const util::Callback& callback, 
		 unsigned int direction)
{
    /* Unlike timers, Qt (4.4) doesn't support QSocketNotifiers
     * constructed on the wrong thread.
     */
    CheckThread();

//    TRACE << "Creating notifier on " << pthread_self() << "\n";

    Notifier *n = (direction == IN)
	? new Notifier(p->GetReadHandle(), callback, QSocketNotifier::Read)
	: new Notifier(p->GetWriteHandle(), callback, QSocketNotifier::Write);
    m_map[p] = n;
}

void Poller::Remove(util::Pollable *p)
{
    CheckThread();
    delete m_map[p];
    m_map.erase(p);
}

void Poller::Add(time_t first, unsigned int repeatms, util::Timed *callback)
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

void Poller::Remove(util::Timed *callback)
{
    delete m_timermap[callback];
    boost::mutex::scoped_lock lock(m_mutex);
    m_timermap.erase(callback);
}

void Poller::Wake()
{
    /* Probably needn't do anything, as won't be used seriously from other
     * threads anyway.
     */
}

} // namespace choraleqt
