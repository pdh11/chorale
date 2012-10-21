#ifndef UPNP_DEVICE_H
#define UPNP_DEVICE_H 1

#include <set>
#include <map>
#include <string>
#include "soap.h"
#include "libutil/observable.h"

namespace upnp {

class Service;

class Server;

/** Base class for UPnP server device implementations.
 *
 * A UPnP server contains one or more Devices, each of which offers
 * one or more Services.
 *
 * Certain UPnP devices -- including their contained services, types,
 * and ids -- are defined by standards; e.g. MediaServer.
 */
class Device
{
    Server *m_server;
    const char *m_device_type;
    typedef std::vector<Service*> services_t;
    services_t m_services;
    std::string m_udn;
    std::string m_friendly_name;

    friend class Service;

    void RegisterService(Service*);
    void FireEvent(Service*, const char *variable, const std::string& value);

public:
    explicit Device(const char *deviceType);
    virtual ~Device() {}

    unsigned int Init(Server*, const std::string& resource);

    Service *FindServiceByID(const char *service_id) const;

    void SetFriendlyName(const std::string& s) { m_friendly_name = s; }

    const std::string& GetUDN() const { return m_udn; }
    const char *GetDeviceType() const { return m_device_type; }
    std::string GetFriendlyName() const { return m_friendly_name; }

    typedef std::vector<Service*>::const_iterator const_iterator;
    const_iterator begin() const { return m_services.begin(); }
    const_iterator end() const { return m_services.end(); }
};


/** Information about an offered UPnP service
 */
class Service: public soap::Server
{
    Device *m_device;
    const char *m_service_id;
    const char *m_service_type;
    const char *m_scpd_url;

public:
    Service(Device*, const char *service_id,
	    const char *service_type, const char *scpd_url);

    const char *GetServiceID() const { return m_service_id; }
    const char *GetServiceType() const { return m_service_type; }
    const char *GetSCPDUrl() const { return m_scpd_url; }

    // soap::Server::OnAction() not defined

    void FireEvent(const char *variable, const std::string& value);
    void FireEvent(const char *variable, unsigned int value);

    virtual void GetEventedVariables(soap::Outbound *vars) = 0;
};

} // namespace upnp

#endif
