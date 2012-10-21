#include "dbus.h"
#include "trace.h"
#include "poll.h"
#include <dbus/dbus.h>
#include <map>
#include <list>
#include <errno.h>

namespace util {

namespace dbus {


        /* Connection::Impl */


class Connection::Impl: public util::Pollable
{
    util::PollerInterface *m_poller;
    DBusError m_err;
    DBusConnection *m_conn;
    util::PollWaker m_waker;

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

    /** Being a util::Pollable */
    unsigned OnActivity();

public:
    Impl(util::PollerInterface *poller);
    ~Impl();

    unsigned int Connect(unsigned int bus);
    unsigned int AddSignalObserver(const std::string& interface,
				   SignalObserver *obs);

    void *GetUnderlyingConnection() { return (void*)m_conn; }
};

Connection::Impl::Impl(util::PollerInterface *poller)
    : m_poller(poller),
      m_conn(NULL),
      m_waker(poller, this)
{
    dbus_error_init(&m_err);
}

Connection::Impl::~Impl()
{
    // Don't close the connection -- it belongs to libdbus not us
}

unsigned int Connection::Impl::Connect(unsigned int bus)
{
    m_conn = dbus_bus_get(bus == SESSION ? DBUS_BUS_SESSION : DBUS_BUS_SYSTEM,
			  &m_err);
    if (dbus_error_is_set(&m_err))
    {
	TRACE << "Can't connect to dbus: " << m_err.message;
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

dbus_bool_t Connection::Impl::StaticAddWatch(DBusWatch *watch, void *handle)
{
    Connection::Impl *self = (Connection::Impl*)handle;
    return self->OnAddWatch(watch);
}

void Connection::Impl::StaticRemoveWatch(DBusWatch *watch, void *handle)
{
    Connection::Impl *self = (Connection::Impl*)handle;
    self->OnRemoveWatch(watch);
}

void Connection::Impl::StaticWatchToggled(DBusWatch *watch, void *handle)
{
    Connection::Impl *self = (Connection::Impl*)handle;
    self->OnWatchToggled(watch);
}

DBusHandlerResult Connection::Impl::StaticHandleMessage(DBusConnection *conn,
							DBusMessage *msg,
							void *handle)
{
    Connection::Impl *self = (Connection::Impl*)handle;
    return self->OnHandleMessage(conn, msg);
}

void Connection::Impl::StaticWakeupMain(void *handle)
{
    Connection::Impl *self = (Connection::Impl*)handle;
    return self->OnWakeupMain();
}

dbus_bool_t Connection::Impl::OnAddWatch(DBusWatch *watch)
{
//    TRACE << "addwatch fd=" << dbus_watch_get_fd(watch)
//	  << " flags=" << dbus_watch_get_flags(watch)
//	  << " enabled=" << dbus_watch_get_enabled(watch) << "\n";
    m_watches.push_back(watch);
    if (dbus_watch_get_enabled(watch))
    {
	unsigned int direction = 0;
	unsigned int watchflags = dbus_watch_get_flags(watch);
	if (watchflags & DBUS_WATCH_READABLE)
	    direction |= util::PollerInterface::IN;
	if (watchflags & DBUS_WATCH_WRITABLE)
	    direction |= util::PollerInterface::OUT;
	m_poller->AddHandle(dbus_watch_get_unix_fd(watch), this, direction);
    }
    return true;
}

void Connection::Impl::OnRemoveWatch(DBusWatch *watch)
{
    TRACE << "removewatch\n";
    m_watches.remove(watch);
    m_poller->RemoveHandle(dbus_watch_get_unix_fd(watch));
}

void Connection::Impl::OnWatchToggled(DBusWatch *watch)
{
    TRACE << "togglewatch\n";
    if (dbus_watch_get_enabled(watch))
    {
	unsigned int direction = 0;
	unsigned int watchflags = dbus_watch_get_flags(watch);
	if (watchflags & DBUS_WATCH_READABLE)
	    direction |= util::PollerInterface::IN;
	if (watchflags & DBUS_WATCH_WRITABLE)
	    direction |= util::PollerInterface::OUT;
	m_poller->AddHandle(dbus_watch_get_unix_fd(watch), this, direction);
    }
    else
    {
	m_poller->RemoveHandle(dbus_watch_get_unix_fd(watch));
    }
}

DBusHandlerResult Connection::Impl::OnHandleMessage(DBusConnection*,
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

void Connection::Impl::OnWakeupMain()
{
//    TRACE << "wakeup\n";
    m_waker.Wake();
}

unsigned int Connection::Impl::OnActivity()
{
//    TRACE << "dbus activity\n";
    for (watches_t::const_iterator i = m_watches.begin();
	 i != m_watches.end();
	 ++i)
    {
	if (dbus_watch_get_enabled(*i))
	    dbus_watch_handle(*i, 3);
    }

    while (dbus_connection_dispatch(m_conn) == DBUS_DISPATCH_DATA_REMAINS)
    {
	/* skip */
    }

    return 0;
}

unsigned int Connection::Impl::AddSignalObserver(const std::string& interface,
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


Connection::Connection(util::PollerInterface *poller)
    : m_impl(new Impl(poller))
{
}

Connection::~Connection()
{
    delete m_impl;
}

unsigned int Connection::Connect(unsigned int bus)
{
    return m_impl->Connect(bus);
}

unsigned int Connection::AddSignalObserver(const std::string& interface,
					   SignalObserver *obs)
{
    return m_impl->AddSignalObserver(interface, obs);
}

void *Connection::GetUnderlyingConnection()
{
    return m_impl->GetUnderlyingConnection();
}

}; // namespace dbus

}; // namespace util

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
    util::Poller poller;
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
		poller.Poll((finish-now)*1000);
	    }
	} while (now < finish);
    }
    else
	TRACE << "Can't connect\n";

    return 0;
}

#endif
