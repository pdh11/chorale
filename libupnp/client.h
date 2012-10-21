#ifndef LIBUPNP_CLIENT_H
#define LIBUPNP_CLIENT_H 1

#include <string>
#include "libutil/bind.h"

namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }
namespace util { class Scheduler; }

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
     * receive GENA notifications through it. The scheduler is used
     * for asynchronous SOAP.
     */
    DeviceClient(util::http::Client*, util::http::Server*,
		 util::Scheduler*);
    ~DeviceClient();

    /** Synchronous initialisation (may block).
     *
     * You probably want the other Init instead.
     */
    unsigned int Init(const std::string& description_url, 
		      const std::string& device_udn);

    /** Asynchronous initialisation.
     *
     * The callback will be called (with an error code or 0 as its
     * parameter) if and only if Init() returns 0.
     */
    typedef util::Callback1<unsigned int> InitCallback;
    unsigned int Init(const std::string& description_url, 
		      const std::string& device_udn,
		      InitCallback callback);

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

    /** Synchronous initialisation (may block).
     *
     * You probably want the other Init instead.
     */
    unsigned int Init();

    /** Asynchronous initialisation.
     *
     * The callback will be called (with an error code or 0 as its
     * parameter) if and only if Init() returns 0.
     */
    typedef util::Callback1<unsigned int> InitCallback;
    unsigned int Init(InitCallback callback);

    /* Synchronous SOAP (not usually a good idea) */

    unsigned int SoapAction(const char *action_name,
			    const soap::Outbound& params,
			    soap::Inbound *result = NULL);
    unsigned int SoapAction(const char *action_name,
			    soap::Inbound *result = NULL);

    /* Asynchronous SOAP */

    typedef util::Callback2<unsigned int, const soap::Inbound*> SoapCallback;

    unsigned int SoapAction(const char *action_name,
			    const soap::Outbound& params,
			    SoapCallback callback);
    unsigned int SoapAction(const char *action_name,
			    SoapCallback callback);

    virtual void OnEvent(const char *var, const std::string& value) = 0;
};

} // namespace upnp

#endif
