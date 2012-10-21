#include "config.h"
#include "device.h"
#include "server.h"
#include "libutil/trace.h"
#include <boost/format.hpp>

namespace upnp {

Service::Service(const char *type, const char *scpdurl)
    : m_type(type), m_scpdurl(scpdurl), m_device(NULL)
{
}

void Service::FireEvent(const char *var, const std::string& value)
{
    if (m_device)
	m_device->FireEvent(m_service_id, var, value);
    else
	TRACE << "No event -- no device\n";
}

void Service::FireEvent(const char *var, unsigned int value)
{
    if (m_device)
    {
	std::string s = (boost::format("%u") % value).str();
	m_device->FireEvent(m_service_id, var, s);
    }
    else
	TRACE << "No event -- no device\n";
}

Device::Device(const char *type, const std::string& resource)
    : m_type(type),
      m_resource(resource),
      m_friendly_name(PACKAGE_NAME),
      m_server(NULL)
{
}

void Device::AddService(const char *service_id, Service *service)
{
    m_services[service_id] = service;
    service->m_device = this;
    service->m_service_id = service_id;
}

void Device::AddEmbeddedDevice(Device *edev)
{
    m_devices.insert(edev);
}

Device *Device::FindByUDN(const char *udn)
{
    if (udn == ("uuid:" + m_uuid))
	return this;

    for (devices_t::const_iterator i = m_devices.begin();
	 i != m_devices.end();
	 ++i)
    {
	Device *d = (*i)->FindByUDN(udn);
	if (d)
	    return d;
    }

    TRACE << "can't find device '" << udn << "' uuid='uuid:" << m_uuid << "'\n";
    return NULL;
}

void Device::FireEvent(const char *serviceid, const char *variable,
		       const std::string& value)
{
    if (m_server)
	m_server->FireEvent("uuid:" + m_uuid, serviceid, variable, value);
    else
	TRACE << "No event -- no server\n";
}

} // namespace upnp
