#ifndef QTPOLLER_H
#define QTPOLLER_H 1

#include "libutil/poll.h"
#include "libutil/not_thread_safe.h"
#include <map>
#include <qsocketnotifier.h>
#include <QTimer>
#include <boost/thread/mutex.hpp>

namespace choraleqt {

/** Helper class for Poller which represents a waitable fd.
 */
class Notifier: public QSocketNotifier
{
    Q_OBJECT

    util::Pollable *m_pollable;

public:
    Notifier(int fd, util::Pollable*, unsigned int direction);

public slots:
    void OnActivity(int);
};

/** Helper class for Poller which represents a timer.
 */
class Timer: public QTimer
{
    Q_OBJECT

    util::Timed *m_timed;
    unsigned int m_repeatms;

public:
    Timer(unsigned int repeatms, util::Timed *timed);

public slots:
    void OnTimer();
};

/** A util::PollerInterface (waiting/timer) implementation for use with Qt:
 * callbacks will be made on the UI thread.
 */
class Poller: public util::PollerInterface, public util::NotThreadSafe
{
    typedef std::map<int, QSocketNotifier*> map_t;
    map_t m_map;

    boost::mutex m_mutex;
    typedef std::map<util::Timed*, Timer*> timermap_t;
    timermap_t m_timermap;

public:
    Poller();
    ~Poller();

    // Being a PollerInterface
    void AddHandle(int poll_handle, util::Pollable *callback, 
		   unsigned int direction);
    void RemoveHandle(int poll_handle);

    void AddTimer(time_t first, unsigned int repeatms, util::Timed*);
    void RemoveTimer(util::Timed*);
};

} // namespace choraleqt

#endif
