#ifndef LIBUTIL_IP_H
#define LIBUTIL_IP_H 1

#include <string>
#include <stdint.h>

namespace util {

struct IPAddress
{
    // Network byte order
    uint32_t addr;

    static IPAddress FromDottedQuad(unsigned char a, unsigned char b,
				    unsigned char c, unsigned char d);

    /** Blocking call, could take some time */
    static IPAddress Resolve(const char *hostname);

    static IPAddress FromHostOrder(uint32_t);
    static IPAddress FromNetworkOrder(uint32_t);
    static const IPAddress ANY;
    static const IPAddress ALL;

    std::string ToString() const;

    bool operator==(const IPAddress& other) { return addr == other.addr; }
    bool operator!=(const IPAddress& other) { return addr != other.addr; }
};

struct IPEndPoint
{
    IPAddress      addr;
    unsigned short port; ///< Host byte order

    std::string ToString() const;
};

} // namespace util

#endif
