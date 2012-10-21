#ifndef LIBUPNP_SOAP_INFO_SOURCE_H
#define LIBUPNP_SOAP_INFO_SOURCE_H 1

#include "libutil/socket.h"

namespace upnp {

namespace soap {

/** Things a SOAP server implementation might need to know about the
 * current transaction.
 *
 * For testability, SOAP servers are encouraged to take an InfoSource
 * *pointer*, and do something sensible if it's NULL.
 */
class InfoSource
{
public:
    virtual ~InfoSource() {}

    /** The IP address and port on which the client contacted us.
     */
    virtual util::IPEndPoint GetCurrentEndPoint() = 0;
};

} // namespace soap
} // namespace upnp

#endif
