#include "config.h"
#include "device.h"
#include "server.h"
#include "libutil/trace.h"
#include <boost/format.hpp>

namespace upnp {

Device::Device(const char *device_type)
    : m_server(NULL),
      m_device_type(device_type),
      m_friendly_name(PACKAGE_NAME)
{
}

unsigned int Device::Init(Server *server, const std::string& resource)
{
    m_udn = server->RegisterDevice(this, resource);
    m_server = server;
    return 0;
}

void Device::RegisterService(Service *service)
{
    m_services.push_back(service);
}

Service *Device::FindServiceByID(const char *service_id) const
{
    for (services_t::const_iterator i = m_services.begin();
	 i != m_services.end();
	 ++i)
    {
	if (!strcmp((*i)->GetServiceID(), service_id))
	    return *i;
    }
    return NULL;
}

void Device::FireEvent(Service *service, const char *variable,
		       const std::string& value)
{
    if (!m_server)
	TRACE << "Not initialised\n";
    else
	m_server->FireEvent(service, variable, value);
}


Service::Service(Device* device, const char *service_id, 
		 const char *service_type, const char *scpd_url)
    : m_device(device),
      m_service_id(service_id),
      m_service_type(service_type),
      m_scpd_url(scpd_url)
{
    device->RegisterService(this);
}

void Service::FireEvent(const char *var, const std::string& value)
{
    m_device->FireEvent(this, var, value);
}

void Service::FireEvent(const char *var, unsigned int value)
{
    std::string s = (boost::format("%u") % value).str();
    m_device->FireEvent(this, var, s);
}

} // namespace upnp
