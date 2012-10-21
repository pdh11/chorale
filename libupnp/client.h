#ifndef LIBUPNP_CLIENT_H
#define LIBUPNP_CLIENT_H 1

#include <string>

namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }

namespace upnp {

namespace soap { class Inbound; }
namespace soap { class Outbound; }

class ServiceClient;

class DeviceClient
{
    class Impl;
    Impl *m_impl;

    friend class ServiceClient;

public:
    /** UPnP client for a particular device.
     *
     * We need an HTTP server as well as the client, because we'll
     * receive GENA notifications through it.
     */
    DeviceClient(util::http::Client*, util::http::Server*);
    ~DeviceClient();

    unsigned int Init(const std::string& description_url, 
		      const std::string& device_udn);

    const std::string& GetFriendlyName() const;
};

class ServiceClient
{
    DeviceClient *m_parent;
    const char *m_service_id;

    class SoapXMLObserver;

protected:
    static unsigned int GenaUInt(const std::string&);
    static int GenaInt(const std::string&);
    static bool GenaBool(const std::string&);

public:
    ServiceClient(DeviceClient* parent, const char *service_id);
    virtual ~ServiceClient();

    unsigned int Init();

    unsigned int SoapAction(const char *action_name,
			    const soap::Outbound& params,
			    soap::Inbound *result = NULL);
    unsigned int SoapAction(const char *action_name,
			    soap::Inbound *result = NULL);

    virtual void OnEvent(const char *var, const std::string& value) = 0;
};

} // namespace upnp

#endif
