#ifndef UPNP_DESCRIPTION_H
#define UPNP_DESCRIPTION_H 1

#include <string>
#include <map>

namespace upnp {

struct ServiceData
{
    std::string type;
    std::string id;
    std::string control_url;
    std::string event_url;
    std::string scpd_url;
};

typedef std::map<std::string, ServiceData> Services;

class Description
{
    std::string m_udn;
    std::string m_friendly_name;
    std::string m_presentation_url;
    Services m_services;

public:
    Description() {}
    unsigned Fetch(const std::string& url, const std::string& udn);

    std::string GetUDN() const { return m_udn; }
    const std::string& GetFriendlyName() const { return m_friendly_name; }
    std::string GetPresentationURL() const { return m_presentation_url; }

    const Services& GetServices() const { return m_services; };
};

} // namespace upnp

#endif
