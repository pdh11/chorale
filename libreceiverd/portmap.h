#ifndef LIBRECEIVERD_PORTMAP_H
#define LIBRECEIVERD_PORTMAP_H

#include "rpc.h"

namespace receiverd {

/** Sun RPC port mapper (RFC1057 Appendix A)
 *
 * Note that for Receiver purposes, SSDP tells the Receiver what port Portmap
 * itself is on -- it won't be the standard port 111.
 */
class PortMapper: public RPCObserver
{
    RPCServer m_rpc;
    typedef std::map<uint32_t, unsigned short> portmap_t;
    portmap_t m_portmap;

    enum {
	PMAPPROC_NULL = 0,
	PMAPPROC_GETPORT = 3
    };

public:
    PortMapper(util::PollerInterface*, util::IPFilter*);

    void AddProgram(uint32_t program_number, unsigned short port)
    {
	m_portmap[program_number] = port;
    }

    unsigned short GetPort() { return m_rpc.GetPort(); }

    // Being an RPCObserver
    unsigned int OnRPC(uint32_t proc, const void *args,
		       size_t argslen, void *reply, size_t *replylen);
};

enum {
    PROGRAM_PORTMAP = 100000,
    PROGRAM_NFS     = 100003,
    PROGRAM_MOUNT   = 100005
};

} // namespace receiverd

#endif
