#ifndef LIB_KARMA_PROTOCOLCLIENT_H
#define LIB_KARMA_PROTOCOLCLIENT_H 1

#include "libutil/socket.h"

namespace karma {

class ProtocolClient
{
    util::IPEndPoint m_endpoint;
    util::StreamSocket m_socket;

    template <class REQUEST, class REPLY>
    unsigned Transaction(const REQUEST*, REPLY*);

public:
    ProtocolClient();

    unsigned Init(util::IPEndPoint);

    unsigned GetProtocolVersion(uint32_t *major, uint32_t *minor);
    unsigned RequestIOLock(bool write);
    unsigned GetAllFileDetails(util::Stream *target);
};

} // namespace karma

#endif
