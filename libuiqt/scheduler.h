#ifndef LIBUIQT_SCHEDULER_H
#define LIBUIQT_SCHEDULER_H

#include "libutil/scheduler.h"
#include "libutil/not_thread_safe.h"
#include "libutil/counted_pointer.h"
#include "libutil/task.h"
#include "libutil/bind.h"
#include <QTimer>
#include <QSocketNotifier>
#include <map>

/** User-interface classes */
namespace ui {

/** User-interface classes for Qt-based user interfaces */
namespace qt {

class Scheduler;

class Timer: public QTimer
{
    Q_OBJECT

    Scheduler *m_scheduler;
    unsigned int m_repeatms;
    util::TaskCallback m_callback;
    bool m_first;

private slots:
    void OnTimer();

public:
    Timer(Scheduler *parent, unsigned int repeatms, util::TaskCallback cb);
    ~Timer();
};

class SocketNotifier: public QSocketNotifier
{
    Q_OBJECT

    int m_pollable;
    Scheduler *m_scheduler;
    util::TaskCallback m_callback;
    bool m_oneshot;

private slots:
    void OnActivity(int);

public:
    SocketNotifier(int pollable, QSocketNotifier::Type direction,
		   Scheduler *scheduler, const util::TaskCallback& callback,
		   bool oneshot);
    ~SocketNotifier();

    util::TaskPtr GetTask() const { return m_callback.GetPtr(); }
};

class Scheduler: public util::Scheduler, public util::NotThreadSafe
{
    util::Mutex m_mutex;
    typedef std::map<util::Task*, Timer*> timermap_t;
    timermap_t m_timermap;
    typedef std::map<int, SocketNotifier*> notifiermap_t;
    notifiermap_t m_notifiermap;

public:
    Scheduler();
    ~Scheduler();

    void RemoveTimer(util::Task*);
    void RemoveSocketNotifier(int);

    // Being a Scheduler
    void WaitForReadable(const util::TaskCallback&, int pollable, 
			 bool oneshot);
    void WaitForWritable(const util::TaskCallback&, int pollable,
			 bool oneshot);
    void Wait(const util::TaskCallback&, time_t first, unsigned int repeatms);
    void Remove(util::TaskPtr);
};

} // namespace ui::qt
} // namespace ui

#endif
