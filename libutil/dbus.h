#ifndef LIBUTIL_DBUS_H
#define LIBUTIL_DBUS_H 1

#include <string>
#include <list>
#include "libutil/counted_pointer.h"

namespace util {

class Scheduler;

/** Classes for interacting with the D-Bus IPC mechanism, used by HAL.
 */
namespace dbus {

class SignalObserver
{
public:
    virtual ~SignalObserver() {}

    virtual void OnSignal(const std::string&) = 0;
};

class MethodObserver
{
public:
    virtual ~MethodObserver() {}
    typedef std::list<std::string> results_t;
    virtual void OnResult(const results_t&) = 0;
};

class Connection
{
    class Task;
    typedef util::CountedPointer<Task> TaskPtr;
    TaskPtr m_task;

public:
    explicit Connection(util::Scheduler *poller);
    ~Connection();

    enum {
	SESSION,
	SYSTEM
    };

    unsigned int Connect(unsigned int bus);

    unsigned int AddSignalObserver(const std::string& interf, 
				   SignalObserver *obs);
    void RemoveSignalObserver(SignalObserver*);

    unsigned int CallMethod(const std::string& service,
			    const std::string& object,
			    const std::string& interf,
			    const std::string& method,
			    MethodObserver *callback);

    /** Returns the DBusConnection pointer. Probably only
     * util::hal::Context should use this.
     */
    void *GetUnderlyingConnection();
};

} // namespace dbus

} // namespace util

#endif
