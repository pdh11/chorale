#ifndef IP_CONFIG_H
#define IP_CONFIG_H 1

#include "socket.h"
#include <vector>

namespace util {

class IPConfig
{
public:
    struct Interface
    {
	IPAddress address;
	IPAddress broadcast;
	IPAddress netmask;
	unsigned int flags;
    };

    typedef std::vector<Interface> Interfaces;

    static unsigned GetInterfaceList(Interfaces*);
};

} // namespace util

#endif
