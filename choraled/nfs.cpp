#include "nfs.h"
#include "libutil/trace.h"
#include "libutil/file_stream.h"
#include "libreceiverd/nfs.h"
#include "libreceiverd/mount.h"
#include "libreceiverd/tarfs.h"
#include "libreceiverd/portmap.h"
#include <errno.h>

namespace choraled {

class NFSService::Impl
{
    receiverd::PortMapperPtr m_portmap;
    util::TaskPtr m_mountd;
    receiverd::TarFS m_tarfs;
    util::TaskPtr m_nfsd;

public:
    Impl(util::Scheduler *poller, util::IPFilter *filter,
	 util::SeekableStreamPtr arfstream);

    unsigned short GetPort() { return m_portmap->GetPort(); }
};

NFSService::Impl::Impl(util::Scheduler *poller, 
		       util::IPFilter *filter,
		       util::SeekableStreamPtr arfstream)
    : m_portmap(receiverd::PortMapper::Create(poller, filter)),
      m_mountd(receiverd::Mount::Create(poller, filter, m_portmap.get())),
      m_tarfs(arfstream),
      m_nfsd(receiverd::NFSServer::Create(poller, filter, m_portmap.get(),
					  &m_tarfs))
{
}

NFSService::NFSService()
    : m_impl(NULL)
{
}

NFSService::~NFSService()
{
    delete m_impl;
}

unsigned int NFSService::Init(util::Scheduler *poller,
			      util::IPFilter *filter,
			      const char *arf)
{
    if (m_impl)
	return EEXIST;

    util::SeekableStreamPtr stm;
    unsigned int rc = util::OpenFileStream(arf, util::READ, &stm);
    if (rc != 0)
    {
	TRACE << "Can't open ARF " << arf << ": " << rc << "\n";
	return rc;
    }

    m_impl = new Impl(poller, filter, stm);
    TRACE << "NFS Init OK\n";
    return 0;
}

unsigned short NFSService::GetPort()
{
    if (!m_impl)
	return 0;
    return m_impl->GetPort();
}

} // namespace choraled
