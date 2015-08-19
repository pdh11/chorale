#ifndef LIBRECEIVER_MOUNT_H
#define LIBRECEIVER_MOUNT_H 1

#include "rpc.h"

namespace receiverd {

class PortMapper;

/** Sun NFS mount daemon (RFC1094 Appendix A)
 *
 * Not by any means a full implementation -- just the parts a Receiver uses.
 */
class Mount: public RPCServer
{
    enum {
	MOUNTPROC_NULL = 0,
	MOUNTPROC_MNT  = 1,
	MOUNTPROC_UMNT = 3
    };

    unsigned int OnRPC(uint32_t proc, const void *args,
		       size_t argslen, void *reply, size_t *replylen) override;

    Mount(util::Scheduler*, util::IPFilter*, PortMapper*);

public:
    
    static util::TaskPtr Create(util::Scheduler*, util::IPFilter*, 
				PortMapper*);
};

} // namespace receiverd

#endif
