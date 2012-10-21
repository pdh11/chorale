#ifndef LIBUPNP_SOAP_H
#define LIBUPNP_SOAP_H

#include <string>
#include <stdint.h>

/** Classes implementing UPnP.
 */
namespace upnp {

struct Data;

/** Classes implementing SOAP, the UPnP RPC protocol.
 */
namespace soap {

struct Params
{
    enum { MAX_PARAMS = 8 }; ///< Max number of any one size

    uint8_t bytes[MAX_PARAMS];
    uint16_t shorts[MAX_PARAMS];
    uint32_t ints[MAX_PARAMS];
    std::string strings[MAX_PARAMS];

    Params();
    ~Params();
};

/** Creating XML from a SOAP parameter structure
 */
std::string CreateBody(const upnp::Data*, unsigned int action, bool response,
		       const char *service_type, const Params& params);

/** A SOAP server.
 *
 * For the client implementation, see upnp::Client.
 */
class Server
{
public:
    virtual ~Server() {}

    virtual unsigned int OnAction(unsigned int action_index,
				  const Params& args,
				  Params *result) = 0;
};

bool ParseBool(const std::string& s);

unsigned int ParseEnum(const std::string& s, const char *const *alternatives,
		       unsigned int count);
unsigned int ParseEnum(const char *s, const char *const *alternatives,
		       unsigned int count);

} // namespace soap
} // namespace upnp

#endif
