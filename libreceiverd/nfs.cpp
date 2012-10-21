#include "nfs.h"
#include "portmap.h"
#include "vfs.h"
#include "libutil/trace.h"
#include "libutil/endian.h"
#include "libutil/counted_pointer.h"
#include <errno.h>
#include <string.h>

namespace receiverd {

namespace nfs {

/** RFC1094 2.3.4 calls it "timeval", but we don't want to clash with time.h
 */
struct Timeval
{
    uint32_t seconds;
    uint32_t useconds;
};

struct Fattr
{
    uint32_t type;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t blocksize;
    uint32_t rdev;
    uint32_t blocks;
    uint32_t fsid;
    uint32_t fileid;
    Timeval atime;
    Timeval mtime;
    Timeval ctime;
};

struct Attrstat
{
    uint32_t status;
    Fattr attributes;
};

struct Fhandle
{
    uint32_t fileid;
    unsigned char pad[28];
};

struct Diropargs
{
    Fhandle dir;
    uint32_t name; // Length followed by data
};

struct Diropres
{
    uint32_t status; // if !=0, this is the only field
    Fhandle file;
    Fattr attributes;
};

struct Readargs
{
    Fhandle file;
    uint32_t offset;
    uint32_t count;
    uint32_t totalcount;
};

} // namespace nfs

util::TaskPtr NFSServer::Create(util::Scheduler *poller, 
				util::IPFilter *filter, PortMapper *portmap,
				VFS *vfs)
{
    return util::TaskPtr(new NFSServer(poller, filter, portmap, vfs));
}

NFSServer::NFSServer(util::Scheduler *poller, util::IPFilter *filter,
		     PortMapper *portmap, VFS *vfs)
    : RPCServer(PROGRAM_NFS, 2, poller, filter),
      m_vfs(vfs)
{
    portmap->AddProgram(PROGRAM_NFS, GetPort());
}

unsigned int NFSServer::OnRPC(uint32_t proc, const void *args,
			      size_t argslen, void *reply, size_t *replylen)
{
    switch (proc)
    {
    case NFSPROC_GETATTR:
	if (argslen >= sizeof(nfs::Fhandle))
	{
	    nfs::Fhandle *fhandle = (nfs::Fhandle*)args;
	    unsigned int fileid = fhandle->fileid;

//	    TRACE << "Alleged getattr is:\n" << Hex(args, argslen);

	    unsigned int type, mode, size, mtime, devt;
	    unsigned int rc = m_vfs->Stat(fhandle->fileid, 
					  &type, &mode, &size, &mtime, &devt);
	    nfs::Attrstat *attrstat = (nfs::Attrstat*)reply;
	    if (rc != 0)
	    {
		TRACE << "NFS stat failed\n";
		attrstat->status = cpu_to_be32(rc);
		*replylen = sizeof(uint32_t);
	    }
	    else
	    {
//		TRACE << "NFS stat succeeded\n";
		memset(attrstat, '\0', sizeof(attrstat));
		attrstat->attributes.type = cpu_to_be32(type);
		attrstat->attributes.mode = cpu_to_be32(mode);
		attrstat->attributes.nlink = cpu_to_be32(1);
		attrstat->attributes.size = cpu_to_be32(size);
		attrstat->attributes.rdev = cpu_to_be32(devt);
		attrstat->attributes.fileid = fileid;
		attrstat->attributes.mtime.seconds = cpu_to_be32(mtime);
		*replylen = sizeof(nfs::Attrstat);
	    }
	    return 0;
	}
	break;

    case NFSPROC_LOOKUP:
	if (argslen >= sizeof(nfs::Diropargs))
	{
	    nfs::Diropargs *diropargs = (nfs::Diropargs*)args;
	    unsigned int fileid = diropargs->dir.fileid;
	    std::string child = String(&diropargs->name,
				       argslen - sizeof(nfs::Diropargs));

	    std::string parent;
	    unsigned int childid;
	    unsigned int type, mode, size, mtime, devt;
	    unsigned int rc = m_vfs->GetNameForHandle(fileid, &parent);
	    if (rc == 0)
		rc = m_vfs->GetHandleForName(parent + child, &childid);
	    if (rc == 0)
		rc = m_vfs->Stat(childid, &type, &mode, &size, &mtime, &devt);

	    nfs::Diropres *diropres = (nfs::Diropres*)reply;
	    if (rc != 0)
	    {
		TRACE << "Looking up '" << child << "' in file " << fileid
		      << " failed\n";
		diropres->status = cpu_to_be32(rc);
		*replylen = sizeof(uint32_t);
	    }
	    else
	    {
//		TRACE << "Looking up '" << child << "' in file " << fileid
//		      << " succeeded (" << childid << ")\n";
		memset(diropres, '\0', sizeof(nfs::Diropres));
		diropres->file.fileid = childid;
		diropres->attributes.type = cpu_to_be32(type);
		diropres->attributes.mode = cpu_to_be32(mode);
		diropres->attributes.nlink = cpu_to_be32(1);
		diropres->attributes.size = cpu_to_be32(size);
		diropres->attributes.rdev = cpu_to_be32(devt);
		diropres->attributes.fileid = childid;
		diropres->attributes.mtime.seconds = cpu_to_be32(mtime);
		*replylen = sizeof(nfs::Diropres);
	    }

	    return 0;
	}
	break;

    case NFSPROC_READLINK:
	if (argslen >= sizeof(nfs::Fhandle))
	{
	    nfs::Fhandle *link = (nfs::Fhandle*)args;
	    std::string s;
	    unsigned int rc = m_vfs->ReadLink(link->fileid, &s);
	    if (rc == 0)
	    {
		uint32_t *uireply = (unsigned int*)reply;
		*uireply = 0;
		uireply[1] = cpu_to_be32((uint32_t)s.length());
		strncpy((char*)(uireply+2), s.c_str(), s.length()+4);
		*replylen = 8 + ((s.length() + 3) & ~3);
	    }
	    else
	    {
		unsigned int *uireply = (unsigned int*)reply;
		*uireply = cpu_to_be32(rc);
		*replylen = sizeof(uint32_t);
	    }
	    return 0;
	}
	break;

    case NFSPROC_READ:
	if (argslen >= sizeof(nfs::Readargs))
	{
	    nfs::Readargs *readargs = (nfs::Readargs*)args;
	    unsigned int fileid = readargs->file.fileid;
	    unsigned int offset = be32_to_cpu(readargs->offset);
	    unsigned int count = be32_to_cpu(readargs->count);
	    unsigned int nread;

	    unsigned int type, mode, size, mtime, devt;
	    unsigned int rc = m_vfs->Stat(fileid, &type, &mode, &size, &mtime,
					  &devt);

	    nfs::Attrstat *attrstat = (nfs::Attrstat*)reply;
	    uint32_t *replycount = (uint32_t*)(attrstat+1);
	    if (rc == 0)
	    {
		if (offset + count > size)
		{
//		    TRACE << "Clipping " << (offset+count) << " bytes to "
//			  << size << "\n";
		    if (offset >= size)
			count = 0;
		    else
			count = size - offset;
		}
		
		rc = m_vfs->Read(fileid, offset, replycount+1, count, &nread);
	    }
	    if (rc == 0)
	    {
		memset(attrstat, '\0', sizeof(nfs::Attrstat));
		
		attrstat->attributes.type = cpu_to_be32(type);
		attrstat->attributes.mode = cpu_to_be32(mode);
		attrstat->attributes.nlink = cpu_to_be32(1);
		attrstat->attributes.size = cpu_to_be32(size);
		attrstat->attributes.rdev = cpu_to_be32(devt);
		attrstat->attributes.fileid = fileid;
		attrstat->attributes.mtime.seconds = cpu_to_be32(mtime);

		*replycount = cpu_to_be32(nread);
		*replylen = sizeof(nfs::Attrstat) + sizeof(uint32_t)
		    + ((nread+3) & ~3);

//		TRACE << "Read returns " << nread << "\n";
	    }
	    else
	    {
		attrstat->status = cpu_to_be32(rc);
		*replylen = sizeof(uint32_t);
	    }
	    return 0;
	}
	break;

    default:
	TRACE << "Didn't like NFS function " << proc << "\n";
	break;
    }

    return ENOTTY;
}

} // namespace receiverd

#ifdef TEST
 
# include "libreceiver/ssdp.h"
# include "mount.h"
# include "tarfs.h"
# include "libutil/scheduler.h"
# include "libutil/file_stream.h"

int main(int argc, char *argv[])
{
    if (argc >= 2)
    {
	util::BackgroundScheduler poller;
	receiverd::PortMapperPtr pmap = receiverd::PortMapper::Create(&poller,
								      NULL);
	receiverd::Mount::Create(&poller, NULL, pmap.get());

	std::auto_ptr<util::Stream> stm;
	unsigned int rc = util::OpenFileStream(argv[1], util::READ, &stm);
	if (rc != 0)
	{
	    TRACE << "Can't open " << argv[1] << ": " << rc << "\n";
	    return 1;
	}

	receiverd::TarFS tarfs(stm.get());

	receiverd::NFSServer::Create(&poller, NULL, pmap.get(), &tarfs);

	receiver::ssdp::Server ssdp(NULL);
	ssdp.Init(&poller);
	ssdp.RegisterService(receiver::ssdp::s_uuid_softwareserver, 
			     pmap->GetPort());

	while (1)
	{
	    poller.Poll(1000);
	}
    }

    return 0;
}

#endif
