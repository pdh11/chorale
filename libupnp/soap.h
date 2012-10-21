#ifndef LIBUPNP_SOAP_H
#define LIBUPNP_SOAP_H

#include <map>
#include <string>

/** Classes implementing UPnP.
 */
namespace upnp {

/** Classes implementing SOAP, the UPnP RPC protocol.
 */
namespace soap {

typedef std::map<std::string, std::string> Params;

/** A SOAP client.
 */
class Connection
{
    class Impl;
    Impl *m_impl;
public:
    Connection(const std::string& url,
	       const std::string& udn,
	       const char *service);
    ~Connection();

    Params Action(const char *action_name,
		  const Params& params);
};

/** A SOAP server.
 */
class Server
{
public:
    virtual ~Server() {}

    virtual Params OnAction(const char *action_name, const Params& params) = 0;
};


/** Helper routine to parse a SOAP boolean.
 */
bool ParseBool(const std::string& s);

}; // namespace soap
}; // namespace upnp

#endif
