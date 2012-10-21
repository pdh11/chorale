#ifndef LIBUTIL_PARTIAL_URL_H
#define LIBUTIL_PARTIAL_URL_H 1

#include "socket.h"

namespace util {

/** A port and path: an HTTP URL minus the IP address.
 *
 * Passing these rather than full URLs helps make multi-homed hosts work.
 */
class PartialURL
{
    std::string m_path;
    unsigned short m_port; // Host byte order
public:
    PartialURL(const std::string& path, unsigned short port)
	: m_path(path),
	  m_port(port)
    {
    }

    std::string Resolve(IPAddress ip)
    {
	IPEndPoint ipe;
	ipe.addr = ip;
	ipe.port = m_port;
	return "http://" + ipe.ToString() + m_path;
    }
};

} // namespace util

#endif
