#ifndef LIBRECEIVER_NFS_H
#define LIBRECEIVER_NFS_H 1

#include "rpc.h"
#include "libutil/task.h"

namespace receiverd {

class PortMapper;
class VFS;

/** Sun NFS daemon (RFC1094)
 *
 * Not by any means a full implementation -- just the parts a Receiver uses.
 */
class NFSServer: public RPCServer
{
    VFS *m_vfs;

    enum {
	NFSPROC_GETATTR = 1,
	NFSPROC_LOOKUP = 4,
	NFSPROC_READLINK = 5,
	NFSPROC_READ = 6
    };

    NFSServer(util::Scheduler*, util::IPFilter*, PortMapper*, VFS*);

    // Being an RPCServer
    unsigned int OnRPC(uint32_t proc, const void *args,
		       size_t argslen, void *reply, size_t *replylen) override;

public:
    static util::TaskPtr Create(util::Scheduler*, util::IPFilter*, PortMapper*,
				VFS*);
};

} // namespace receiverd

#endif
