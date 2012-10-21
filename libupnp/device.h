#ifndef UPNP_DEVICE_H
#define UPNP_DEVICE_H 1

#include <set>
#include <map>
#include <string>
#include "soap.h"
#include "libutil/observable.h"

namespace upnp {

class Device;

/** Information about an offered UPnP service
 */
class Service: public soap::Server
{
    const char *m_type;
    const char *m_scpdurl;
    Device *m_device;
    const char *m_service_id;

    friend class Device;

public:
    Service(const char *type, const char *scpdurl);

    const char *GetType() const { return m_type; }
    const char *GetSCPDUrl() const { return m_scpdurl; }

    // soap::Server::OnAction() not defined

    void FireEvent(const char *variable, const std::string& value);
    void FireEvent(const char *variable, unsigned int value);

    virtual void GetEventedVariables(soap::Outbound *vars) = 0;
};

class Server;

class Device
{
    const char *m_type;
    typedef std::set<Device*> devices_t;
    devices_t m_devices;
    typedef std::map<std::string, Service*> services_t;
    services_t m_services;
    std::string m_resource;
    std::string m_uuid;
    std::string m_friendly_name;
    Server *m_server;

    friend class Server;

public:
    Device(const char *deviceType, const std::string& resource);
    virtual ~Device() {}

    void AddService(const char *serviceID,
		    Service *service);

    void AddEmbeddedDevice(Device*);
    void SetFriendlyName(const std::string& s) { m_friendly_name = s; }

    Device *FindByUDN(const char *udn);
    std::string UDN() const { return "uuid:" + m_uuid; }

    void FireEvent(const char *serviceid, const char *variable,
		   const std::string& value);
};

} // namespace upnp

#endif
