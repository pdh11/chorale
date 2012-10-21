#ifndef LIBUPNP_CLIENT_H
#define LIBUPNP_CLIENT_H 1

#include "device.h"

namespace upnp {

class Description;

namespace soap { class Inbound; }
namespace soap { class Outbound; }

class ClientConnection;

class Client
{
    class Impl;
    Impl *m_impl;

    friend class ClientConnection;

public:
    Client();
    ~Client();

    unsigned int Init(const std::string& description_url, 
		      const std::string& device_udn);

    const Description& GetDescription() const;
};

class ClientConnection
{
    class Impl;
    Impl *m_impl;

protected:
    static unsigned int GenaUInt(const std::string&);
    static int GenaInt(const std::string&);
    static bool GenaBool(const std::string&);

public:
    ClientConnection();
    virtual ~ClientConnection();

    void SetSid(const std::string& sid);

    unsigned int Init(Client* parent, const char *service_id);

    unsigned int SoapAction(const char *action_name,
			    const soap::Outbound& params,
			    soap::Inbound *result);

    virtual void OnEvent(const char *var, const std::string& value) = 0;
};

} // namespace upnp

#endif
