#include "dbus.h"
#include "config.h"
#include "trace.h"
#include "bind.h"
#include "scheduler.h"
#include "task.h"
#include "pollable.h"
#include <map>
#include <list>
#include <errno.h>

#undef IN
#undef OUT

#if HAVE_DBUS

#include <dbus/dbus.h>

namespace util {

namespace dbus {

class WatchPollable: public util::Pollable
{
    int m_fd;

public:
    WatchPollable() : m_fd(-1) {}

    void SetHandle(int fd) { m_fd = fd; }

    // Being a Pollable
    PollHandle GetHandle() { return m_fd; }
};


        /* Connection::Task */


class Connection::Task: public util::Task
{
    util::Scheduler *m_poller;
    DBusError m_err;
    DBusConnection *m_conn;
    WatchPollable m_pollable;

    typedef std::map<std::string, SignalObserver*> map_t;
    map_t m_map;

    typedef std::list<DBusWatch*> watches_t;
    watches_t m_watches;

    /* libdbus C callbacks */
    static dbus_bool_t StaticAddWatch(DBusWatch*, void*);
    static void StaticRemoveWatch(DBusWatch*, void*);
    static void StaticWatchToggled(DBusWatch*, void*);
    static DBusHandlerResult StaticHandleMessage(DBusConnection*, DBusMessage*,
						 void*);
    static void StaticWakeupMain(void*);

    /* C++ callbacks */
    dbus_bool_t OnAddWatch(DBusWatch*);
    void OnRemoveWatch(DBusWatch*);
    void OnWatchToggled(DBusWatch*);
    DBusHandlerResult OnHandleMessage(DBusConnection*, DBusMessage*);
    void OnWakeupMain();

    unsigned Run();

    typedef CountedPointer<Task> TaskPtr;

public:
    Task(util::Scheduler *poller);
    ~Task();

    unsigned int Connect(unsigned int bus);
    unsigned int AddSignalObserver(const std::string& interface,
				   SignalObserver *obs);

    void *GetUnderlyingConnection() { return (void*)m_conn; }
};

Connection::Task::Task(util::Scheduler *poller)
    : m_poller(poller),
      m_conn(NULL)
{
    dbus_error_init(&m_err);
}

Connection::Task::~Task()
{
    // Don't close the connection -- it belongs to libdbus not us
}

unsigned int Connection::Task::Connect(unsigned int bus)
{
    m_conn = dbus_bus_get(bus == SESSION ? DBUS_BUS_SESSION : DBUS_BUS_SYSTEM,
			  &m_err);
    if (dbus_error_is_set(&m_err))
    {
	TRACE << "Can't connect to dbus: " << m_err.message << "\n";
	dbus_error_free(&m_err);
	return ENOENT;
    }

    dbus_connection_set_watch_functions(m_conn, &StaticAddWatch,
					&StaticRemoveWatch,
					&StaticWatchToggled, this, NULL);
    dbus_connection_set_wakeup_main_function(m_conn, &StaticWakeupMain, this,
					     NULL);
    dbus_connection_add_filter(m_conn, &StaticHandleMessage, this, NULL);
    return 0;
}

dbus_bool_t Connection::Task::StaticAddWatch(DBusWatch *watch, void *handle)
{
    Connection::Task *self = (Connection::Task*)handle;
    return self->OnAddWatch(watch);
}

void Connection::Task::StaticRemoveWatch(DBusWatch *watch, void *handle)
{
    Connection::Task *self = (Connection::Task*)handle;
    self->OnRemoveWatch(watch);
}

void Connection::Task::StaticWatchToggled(DBusWatch *watch, void *handle)
{
    Connection::Task *self = (Connection::Task*)handle;
    self->OnWatchToggled(watch);
}

DBusHandlerResult Connection::Task::StaticHandleMessage(DBusConnection *conn,
							DBusMessage *msg,
							void *handle)
{
    Connection::Task *self = (Connection::Task*)handle;
    return self->OnHandleMessage(conn, msg);
}

void Connection::Task::StaticWakeupMain(void *handle)
{
    Connection::Task *self = (Connection::Task*)handle;
    return self->OnWakeupMain();
}

dbus_bool_t Connection::Task::OnAddWatch(DBusWatch *watch)
{
//    TRACE << "addwatch fd=" << dbus_watch_get_unix_fd(watch)
//	  << " flags=" << dbus_watch_get_flags(watch)
//	  << " enabled=" << dbus_watch_get_enabled(watch) << "\n";
    
    if (m_pollable.GetHandle() == -1)
	m_pollable.SetHandle(dbus_watch_get_unix_fd(watch));

    m_watches.push_back(watch);
    
    OnWatchToggled(watch); // Sets up waitforread/write
    return true;
}

void Connection::Task::OnWatchToggled(DBusWatch *watch)
{
//    TRACE << "togglewatch fd=" << dbus_watch_get_unix_fd(watch)
//	  << " flags=" << dbus_watch_get_flags(watch)
//	  << " enabled=" << dbus_watch_get_enabled(watch) << "\n";
    
    if (dbus_watch_get_enabled(watch))
    {
	unsigned int watchflags = dbus_watch_get_flags(watch);
	if (watchflags & DBUS_WATCH_READABLE)
	    m_poller->WaitForReadable(
		Bind<Task,&Task::Run>(TaskPtr(this)), &m_pollable);
	else
	    if (watchflags & DBUS_WATCH_WRITABLE)
		m_poller->WaitForWritable(
		    Bind<Task,&Task::Run>(TaskPtr(this)), &m_pollable);
    }
}

void Connection::Task::OnRemoveWatch(DBusWatch *watch)
{
    m_watches.remove(watch);
}

DBusHandlerResult Connection::Task::OnHandleMessage(DBusConnection*,
						    DBusMessage *msg)
{
//    TRACE << "woot, got message\n";

    if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL)
    {
	std::string interface = dbus_message_get_interface(msg);
	std::string member = dbus_message_get_member(msg);
//	TRACE << "woot, it's a signal from '" << interface
//	      << "' member '" << member << "'\n";
	if (m_map.find(interface) != m_map.end())
	{
	    DBusMessageIter args;
	    std::string arg;
	    if (dbus_message_iter_init(msg, &args))
	    {
		if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING)
		    arg = "<not-a-string>";
		else
		{
		    const char *cstr;
		    dbus_message_iter_get_basic(&args, &cstr);
		    arg = cstr;
		}
	    }
	    m_map[interface]->OnSignal(arg);
	    return DBUS_HANDLER_RESULT_HANDLED;
	}
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void Connection::Task::OnWakeupMain()
{
//    TRACE << "wakeup\n";
    m_poller->Wake();
}

unsigned int Connection::Task::Run()
{
//    TRACE << "dbus activity\n";
    for (watches_t::const_iterator i = m_watches.begin();
	 i != m_watches.end();
	 ++i)
    {
	if (dbus_watch_get_enabled(*i))
	{
	    dbus_watch_handle(*i, 3);
	    OnWatchToggled(*i);
	}
    }

    while (dbus_connection_dispatch(m_conn) == DBUS_DISPATCH_DATA_REMAINS)
    {
	/* skip */
    }    

    return 0;
}

unsigned int Connection::Task::AddSignalObserver(const std::string& interface,
						 SignalObserver *obs)
{
    m_map[interface] = obs;
    std::string rule = "type='signal'"; //,interface='" + interface + "'";
    dbus_bus_add_match(m_conn, rule.c_str(), &m_err);
    
    if (dbus_error_is_set(&m_err))
    { 
	TRACE << "match error: " << m_err.message << "\n";
	return EINVAL;
    }
    return 0;
}


        /* Connection itself */


Connection::Connection(util::Scheduler *poller)
    : m_task(new Task(poller))
{
}

Connection::~Connection()
{
}

unsigned int Connection::Connect(unsigned int bus)
{
    return m_task->Connect(bus);
}

unsigned int Connection::AddSignalObserver(const std::string& interface,
					   SignalObserver *obs)
{
    return m_task->AddSignalObserver(interface, obs);
}

void *Connection::GetUnderlyingConnection()
{
    return m_task->GetUnderlyingConnection();
}

} // namespace dbus

} // namespace util

#endif // HAVE_DBUS

#ifdef TEST

class TestObserver: public util::dbus::SignalObserver
{
public:
    void OnSignal(const std::string& signal)
	{
	    TRACE << "got signal: " << signal << "\n";
	}
};

int main()
{
#if HAVE_DBUS
    util::BackgroundScheduler poller;
    util::dbus::Connection conn(&poller);
    unsigned int rc = conn.Connect(util::dbus::Connection::SYSTEM);

    if (rc == 0)
    {
	TestObserver testobs;
	conn.AddSignalObserver("org.freedesktop.Hal.Manager", &testobs);

	time_t start = time(NULL);
	time_t finish = start+2;
	time_t now;

	do {
	    now = time(NULL);
	    if (now < finish)
	    {
//		TRACE << "polling\n";
		poller.Poll((unsigned)(finish-now)*1000);
	    }
	} while (now < finish);
    }
    else
	TRACE << "Can't connect\n";
#endif // HAVE_DBUS

    return 0;
}

#endif // TEST
