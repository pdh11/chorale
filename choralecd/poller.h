#ifndef QTPOLLER_H
#define QTPOLLER_H 1

#include "libutil/poll.h"
#include <map>
#include <qsocketnotifier.h>

namespace choraleqt {

class Notifier: public QSocketNotifier
{
    Q_OBJECT;

    util::Pollable *m_pollable;

public:
    Notifier(int fd, util::Pollable*, unsigned int direction);

public slots:
    void OnActivity(int);
};

/** A PollInterface implementation for use with Qt: callbacks will be made on
 * the UI thread.
 */
class Poller: public util::PollerInterface
{
    typedef std::map<int, QSocketNotifier*> map_t;
    map_t m_map;

public:
    Poller();
    ~Poller();

    // Being a PollerInterface
    void AddHandle(int poll_handle, util::Pollable *callback, 
		   unsigned int direction);
    void RemoveHandle(int poll_handle);
};

}; // namespace choraleqt

#endif
