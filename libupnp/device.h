#ifndef UPNP_DEVICE_H
#define UPNP_DEVICE_H 1

#include <set>
#include <map>
#include <string>

namespace util { class WebServer; };

namespace upnp {

namespace soap { class Server; };

class Service
{
    const char *m_type;
    const char *m_scpdurl;
    soap::Server *m_server;

public:
    Service(const char *type, const char *scpdurl, soap::Server *server);

    const char *GetType() const { return m_type; }
    const char *GetSCPDUrl() const { return m_scpdurl; }
    soap::Server *GetServer() { return m_server; }
};

class Device
{
    const char *m_type;
    typedef std::set<Device*> devices_t;
    devices_t m_devices;
    typedef std::map<std::string, Service*> services_t;
    services_t m_services;
    std::string m_resource;
    std::string m_uuid;

    friend class DeviceManager;

public:
    Device(const char *deviceType, const std::string& resource);

    void AddService(const char *serviceID,
		    Service *service);

    void AddEmbeddedDevice(Device*);

    Device *FindByUDN(const char *udn);
    std::string UDN() const { return "uuid:" + m_uuid; }
};

class DeviceManager
{
    class Impl;
    Impl *m_impl;

public:
    DeviceManager();
    ~DeviceManager();

    unsigned int SetRootDevice(Device*);
};

}; // namespace upnp

#endif
