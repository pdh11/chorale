#ifndef LIBUPNP_SERVER_H
#define LIBUPNP_SERVER_H 1

#include <string>
#include "soap.h"
#include "soap_info_source.h"

namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }
namespace util { class PollerInterface; }

namespace upnp {

namespace ssdp { class Responder; }

class Device;
class Service;

/** A UPnP server.
 *
 * The devices announced by this serve are supplied as subclasses of
 * upnp::Device, which themselves contain subclasses of upnp::Service.
 */
class Server: public upnp::soap::InfoSource
{
    class Impl;
    Impl *m_impl;

    friend class Device;
    /** Returns UDN
     */
    std::string RegisterDevice(Device*, const std::string& resource);
    void FireEvent(Service*, const char *variable, const std::string& value);

public:
    Server(util::PollerInterface*, util::http::Client*, util::http::Server*, 
	   ssdp::Responder*);
    ~Server();

    /** Start up UPnP client, including starting SSDP advertisements.
     */
    unsigned int Init();

    // Being a upnp::soap::InfoSource
    util::IPEndPoint GetCurrentEndPoint();
};

} // namespace upnp

#endif
