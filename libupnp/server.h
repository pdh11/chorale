#ifndef LIBUPNP_SERVER_H
#define LIBUPNP_SERVER_H 1

#include <string>

namespace upnp {

class Device;

class Server
{
    class Impl;
    Impl *m_impl;

public:
    Server();
    ~Server();

    /** Start up UPnP client, including starting SSDP advertisements.
     *
     * @arg presentationport Port number of web server for presentation page.
     */
    unsigned int Init(Device *rootdevice, unsigned short presentationport);

    void FireEvent(const std::string& udn, const char *service_id,
		   const char *variable, const std::string& value);
};

} // namespace upnp

#endif
