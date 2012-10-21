#ifndef LIBUTIL_IP_FILTER_H
#define LIBUTIL_IP_FILTER_H

#include "socket.h"

namespace util {

/** Interface for filtering access by IP address
 */
class IPFilter
{
public:
    virtual ~IPFilter() {}
    
    enum {
	DENY,
	READONLY,
	FULL
    };

    virtual unsigned int CheckAccess(IPAddress ip) = 0;
};

class IPFilterYes: public IPFilter
{
public:
    unsigned int CheckAccess(IPAddress) { return FULL; }
};

} // namespace util

#endif
