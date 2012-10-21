#include "mount.h"
#include "portmap.h"
#include "libutil/trace.h"
#include "libutil/endian.h"
#include <errno.h>

namespace receiverd {

namespace mountprog {

struct FHStatus
{
    uint32_t status;
    char handle[32];
};  

} // namespace mountprog

Mount::Mount(util::PollerInterface *poller, PortMapper *portmap)
    : m_rpc(PROGRAM_MOUNT, 1, poller, this)
{
    portmap->AddProgram(PROGRAM_MOUNT, m_rpc.GetPort());
}

unsigned int Mount::OnRPC(uint32_t proc, const void*,
			  size_t, void *reply, size_t *replylen)
{
    switch (proc)
    {
    case MOUNTPROC_MNT:
    {
	/* Always succeeds */
	mountprog::FHStatus *presult = (mountprog::FHStatus*)reply;
	presult->status = 0;
	memset(presult->handle, '\0', 32);
	*replylen = sizeof(mountprog::FHStatus);

//	TRACE << "Mount reply is:\n" << Hex(reply, *replylen);
	return 0;
    }

    case MOUNTPROC_NULL:
    case MOUNTPROC_UMNT:
	*replylen = 0;
	return 0;

    default:
	TRACE << "Didn't like mount function " << proc << "\n";
	break;
    }

    return ENOTTY;
}

} // namespace receiverd
