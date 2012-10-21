#ifndef IP_CONFIG_H
#define IP_CONFIG_H 1

#include "ip.h"
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
	std::string name;
    };

    typedef std::vector<Interface> Interfaces;

    /** Get the current list of TCP/IP interfaces.
     *
     * Interfaces which are down or have no address are skipped.
     */
    static unsigned GetInterfaceList(Interfaces*);
};

} // namespace util

#endif
