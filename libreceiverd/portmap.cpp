#include "portmap.h"
#include "libutil/trace.h"
#include "libutil/endian.h"
#include "libutil/counted_pointer.h"
#include <errno.h>

namespace receiverd {

namespace portmap {

struct Mapping
{
    uint32_t prog;
    uint32_t vers;
    uint32_t prot;
    uint32_t port;
};

} // namespace portmap

PortMapperPtr PortMapper::Create(util::Scheduler *scheduler, 
				 util::IPFilter *filter)
{
    return PortMapperPtr(new PortMapper(scheduler, filter));
}

PortMapper::PortMapper(util::Scheduler *poller, util::IPFilter *filter)
    : RPCServer(PROGRAM_PORTMAP, 2, poller, filter)
{
}

unsigned int PortMapper::OnRPC(uint32_t proc, const void *args,
			       size_t argslen, void *reply, size_t *replylen)
{
    switch (proc)
    {
    case PMAPPROC_NULL:
	*replylen = 0;
	return 0;

    case PMAPPROC_GETPORT:
	if (argslen >= sizeof(portmap::Mapping))
	{
	    const portmap::Mapping *mapping = (const portmap::Mapping*)args;
	    
	    uint32_t result = 0;
	    
	    if (be32_to_cpu(mapping->prot) == 17) // IPPROTO_UDP
	    {
		portmap_t::const_iterator i =
		    m_portmap.find(be32_to_cpu(mapping->prog));
		if (i == m_portmap.end())
		{
		    TRACE << "Can't find program number "
			  << be32_to_cpu(mapping->prog) << "\n";
		}
		else
		    result = i->second;
	    }
	    else
		TRACE << "Don't like non-UDP portmap request\n";
	    
	    uint32_t *presult = (uint32_t*)reply;
	    *presult = cpu_to_be32(result);
	    *replylen = sizeof(uint32_t);
	    return 0;
	}
	break;

    default:
	TRACE << "Didn't like portmap function " << proc << "\n";
	break;
    }

    return ENOTTY;
}

} // namespace receiverd
