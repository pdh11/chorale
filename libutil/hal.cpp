#include "config.h"
#include "hal.h"
#include "dbus.h"
#include "trace.h"
#include "poll.h"
#include "observable.h"
#include <errno.h>

#if HAVE_HAL

#include <libhal.h>
#include <dbus/dbus.h>

namespace util {

namespace hal {

class Context::Impl: public util::Observable<Observer>
{
    LibHalContext *m_ctx;

    /* Static C callback / C++ callback pairs */

    static void StaticDeviceAdded(LibHalContext *ctx, const char *udi);
    void OnDeviceAdded(const char *udi);

    static void StaticDeviceRemoved(LibHalContext *ctx, const char *udi);
    void OnDeviceRemoved(const char *udi);

public:
    Impl(dbus::Connection*);
    ~Impl();

    unsigned int Init();
    void GetMatchingDevices(const char *key, const char *value, 
			    DeviceObserver *obs);
    std::string DeviceGetString(const char *udi,
					const char *key);
    unsigned int DeviceGetInt(const char *udi,
				      const char *property);
};

Context::Impl::Impl(dbus::Connection *conn)
    : m_ctx(libhal_ctx_new())
{
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
    libhal_ctx_set_device_added(m_ctx, &StaticDeviceAdded);
    libhal_ctx_set_device_removed(m_ctx, &StaticDeviceRemoved);
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
    DevicePtr dp(new Device(this, udi));
    TRACE << "New HAL device " << udi << "\n";
    Fire(&Observer::OnDeviceAdded, dp);
}

void Context::Impl::StaticDeviceRemoved(LibHalContext *ctx, const char *udi)
{
    Impl *self = (Impl*)libhal_ctx_get_user_data(ctx);
    self->OnDeviceRemoved(udi);
}

void Context::Impl::OnDeviceRemoved(const char *udi)
{
    DevicePtr dp(new Device(this, udi));
//    TRACE << "Removing HAL device " << udi << "\n";
    Fire(&Observer::OnDeviceRemoved, dp);
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
    {
	DevicePtr dp(new Device(this, result[i]));
	obs->OnDevice(dp);
    }

    libhal_free_string_array(result);
}

std::string Context::Impl::DeviceGetString(const char *udi,
						   const char *property)
{
    char *cstr = libhal_device_get_property_string(m_ctx, udi, 
						   property, NULL);
    if (!cstr)
	return std::string();

    std::string result = cstr;
    libhal_free_string(cstr);
    return result;
}

unsigned int Context::Impl::DeviceGetInt(const char *udi,
						 const char *property)
{
    return (unsigned)libhal_device_get_property_int(m_ctx, udi, property,
						    NULL);
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

void Context::AddObserver(Observer *obs)
{
    m_impl->AddObserver(obs);
}

void Context::RemoveObserver(Observer *obs)
{
    m_impl->RemoveObserver(obs);
}


        /* Device */


Device::Device(Context::Impl *parent, const char *udi)
    : m_udi(udi),
      m_ctx(parent)
{
}

Device::~Device()
{
}

std::string Device::GetString(const char *key)
{
    return m_ctx->DeviceGetString(m_udi.c_str(), key);
}

unsigned int Device::GetInt(const char *key)
{
    return m_ctx->DeviceGetInt(m_udi.c_str(), key);
}

} // namespace hal

} // namespace util

#endif // HAVE_HAL


        /* Unit tests */


#ifdef TEST

#if HAVE_HAL

class CDObserver: public util::hal::DeviceObserver
{
public:
    void OnDevice(util::hal::DevicePtr dev);
};

void CDObserver::OnDevice(util::hal::DevicePtr dev)
{
    std::string product = dev->GetString("info.product");
    std::string device = dev->GetString("block.device");

    TRACE << "Found CD '" << product << "' at '" << device << "'\n";
}

class SoundObserver: public util::hal::DeviceObserver
{
public:
    void OnDevice(util::hal::DevicePtr dev);
};

void SoundObserver::OnDevice(util::hal::DevicePtr dev)
{
    std::string product = dev->GetString("info.product");
    unsigned card = dev->GetInt("alsa.card");
    unsigned device = dev->GetInt("alsa.device");

    TRACE << "Found ALSA output '" << product << "' at 'hw"
	  << card << "," << device << "'\n";
}

class DVBObserver: public util::hal::DeviceObserver
{
public:
    void OnDevice(util::hal::DevicePtr dev);
};

void DVBObserver::OnDevice(util::hal::DevicePtr dev)
{
    std::string product = dev->GetString("info.product");
    std::string device = dev->GetString("dvb.device");

    if (device.find("frontend") != std::string::npos)
	TRACE << "Found DVB '" << product << "' at '" << device << "'\n";
}

#endif // HAVE_HAL

int main()
{
#if HAVE_HAL
    util::Poller poller;
    util::dbus::Connection conn(&poller);
    unsigned int rc = conn.Connect(util::dbus::Connection::SYSTEM);

    if (rc)
    {
	TRACE << "Can't connect (" << rc << ")\n";
	return 0;
    }
    
    util::hal::Context ctx(&conn);

    rc = ctx.Init();

    CDObserver cdobs;
    ctx.GetMatchingDevices("storage.drive_type", "cdrom", &cdobs);

    SoundObserver sobs;
    ctx.GetMatchingDevices("alsa.type", "playback", &sobs);

    DVBObserver dobs;
    ctx.GetMatchingDevices("info.category", "dvb", &dobs);

#endif // HAVE_HAL

    return 0;
}

#endif // TEST
