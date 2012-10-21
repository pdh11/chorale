#include "nfs.h"
#include "libutil/trace.h"
#include "libutil/file_stream.h"
#include "libreceiverd/nfs.h"
#include "libreceiverd/mount.h"
#include "libreceiverd/tarfs.h"
#include "libreceiverd/portmap.h"
#include <errno.h>

class NFSService::Impl
{
    receiverd::PortMapper m_portmap;
    receiverd::Mount m_mountd;
    receiverd::TarFS m_tarfs;
    receiverd::NFSServer m_nfsd;

public:
    Impl(util::PollerInterface *poller, util::IPFilter *filter,
	 util::SeekableStreamPtr arfstream);

    unsigned short GetPort() { return m_portmap.GetPort(); }
};

NFSService::Impl::Impl(util::PollerInterface *poller, 
		       util::IPFilter *filter,
		       util::SeekableStreamPtr arfstream)
    : m_portmap(poller, filter),
      m_mountd(poller, filter, &m_portmap),
      m_tarfs(arfstream),
      m_nfsd(poller, filter, &m_portmap, &m_tarfs)
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

unsigned int NFSService::Init(util::PollerInterface *poller,
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
