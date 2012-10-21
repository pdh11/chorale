#include "config.h"
#include "hal.h"
#include "dbus.h"
#include "trace.h"
#include "poll.h"
#include <errno.h>

#ifdef HAVE_HAL

#include <libhal.h>
#include <dbus/dbus.h>

namespace util {

namespace hal {

class Context::Impl
{
    LibHalContext *m_ctx;

    typedef std::list<Observer*> obs_t;
    obs_t m_obs;

    /* Static C callback / C++ callback pairs */

    void StaticDeviceAdded(LibHalContext *ctx, const char *udi);
    void OnDeviceAdded(const char *udi);

public:
    Impl(dbus::Connection*);
    ~Impl();

    unsigned int Init();
    void GetMatchingDevices(const char *key, const char *value, 
			    DeviceObserver *obs);
    std::string DeviceGetPropertyString(const char *udi,
					const char *property);
    unsigned int DeviceGetPropertyInt(const char *udi,
				      const char *property);
};

Context::Impl::Impl(dbus::Connection *conn)
{
    m_ctx = libhal_ctx_new();
    if (!m_ctx)
    {
	TRACE << "Can't create libhal context\n";
	return;
    }
    if (!libhal_ctx_set_dbus_connection(m_ctx, 
					(DBusConnection*)conn->GetUnderlyingConnection()))
    {
	TRACE << "Can't set dbus connection\n";
    }

    libhal_ctx_set_user_data(m_ctx, this);
}

Context::Impl::~Impl()
{
    if (m_ctx)
	libhal_ctx_free(m_ctx);
}

unsigned int Context::Impl::Init()
{
    if (!m_ctx)
	return ENOENT;

    if (!libhal_ctx_init(m_ctx, NULL))
    {
	TRACE << "Can't connect to hald\n";
	libhal_ctx_free(m_ctx);
	m_ctx = NULL;
	return ECONNREFUSED;
    }
    return 0;
}

void Context::Impl::StaticDeviceAdded(LibHalContext *ctx, const char *udi)
{
    Impl *self = (Impl*)libhal_ctx_get_user_data(ctx);
    self->OnDeviceAdded(udi);
}

void Context::Impl::OnDeviceAdded(const char *udi)
{
    for (obs_t::const_iterator i = m_obs.begin(); i != m_obs.end(); ++i)
	(*i)->OnDeviceAdded(udi);
}

void Context::Impl::GetMatchingDevices(const char *key, const char *value,
				       DeviceObserver *obs)
{
    int num = 0;
    char **result = libhal_manager_find_device_string_match(m_ctx,
							    key,
							    value,
							    &num, NULL);
    if (!result)
	return;

    for (int i=0; i<num; ++i)
	obs->OnDevice(result[i]);

    libhal_free_string_array(result);
}

std::string Context::Impl::DeviceGetPropertyString(const char *udi,
						   const char *property)
{
    char *cstr = libhal_device_get_property_string(m_ctx, udi, 
						   property, NULL);
    std::string result = cstr;
    libhal_free_string(cstr);
    return result;
}

unsigned int Context::Impl::DeviceGetPropertyInt(const char *udi,
						 const char *property)
{
    return libhal_device_get_property_int(m_ctx, udi, property, NULL);
}


        /* Context itself */


Context::Context(dbus::Connection *conn)
    : m_impl(new Impl(conn))
{
}

Context::~Context()
{
    delete m_impl;
}

unsigned int Context::Init()
{
    return m_impl->Init();
}

void Context::GetMatchingDevices(const char *property,
				 const char *value,
				 DeviceObserver *obs)
{
    m_impl->GetMatchingDevices(property, value, obs);
}

std::string Context::DeviceGetPropertyString(const char *udi,
					     const char *property)
{
    return m_impl->DeviceGetPropertyString(udi, property);
}

unsigned int Context::DeviceGetPropertyInt(const char *udi,
					   const char *property)
{
    return m_impl->DeviceGetPropertyInt(udi, property);
}

}; // namespace hal

}; // namespace util

#endif // HAVE_HAL


        /* Unit tests */


#ifdef TEST

#ifdef HAVE_HAL

class CDObserver: public util::hal::DeviceObserver
{
    util::hal::Context *m_hal;

public:
    explicit CDObserver(util::hal::Context *hal) : m_hal(hal) {}

    void OnDevice(const std::string& udi);
};

void CDObserver::OnDevice(const std::string& udi)
{
    std::string product = m_hal->DeviceGetPropertyString(udi.c_str(), "info.product");
    std::string device = m_hal->DeviceGetPropertyString(udi.c_str(), "block.device");

    TRACE << "Found CD '" << product << "' at '" << device << "'\n";
}

class SoundObserver: public util::hal::DeviceObserver
{
    util::hal::Context *m_hal;

public:
    explicit SoundObserver(util::hal::Context *hal) : m_hal(hal) {}

    void OnDevice(const std::string& udi);
};

void SoundObserver::OnDevice(const std::string& udi)
{
    std::string product = m_hal->DeviceGetPropertyString(udi.c_str(), "info.product");
    unsigned card = m_hal->DeviceGetPropertyInt(udi.c_str(), "alsa.card");
    unsigned device = m_hal->DeviceGetPropertyInt(udi.c_str(), "alsa.device");

    TRACE << "Found ALSA output '" << product << "' at 'hw"
	  << card << "," << device << "'\n";
}

#endif // HAVE_HAL

int main()
{
#ifdef HAVE_HAL
    util::Poller poller;
    util::dbus::Connection conn(&poller);
    unsigned int rc = conn.Connect(util::dbus::Connection::SYSTEM);

    if (rc)
    {
	TRACE << "Can't connect (" << rc << ")\n";
	return 1;
    }
    
    util::hal::Context ctx(&conn);

    rc = ctx.Init();

    CDObserver cdobs(&ctx);

    ctx.GetMatchingDevices("storage.drive_type", "cdrom", &cdobs);

    SoundObserver sobs(&ctx);

    ctx.GetMatchingDevices("alsa.type", "playback", &sobs);
#endif // HAVE_HAL

    return 0;
}

#endif // TEST
