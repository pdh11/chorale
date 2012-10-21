/* libempeg/fid_stream.cpp */

#include "fid_stream.h"
#include "protocol_client.h"
#include <errno.h>

namespace empeg {

FidStream::FidStream(ProtocolClient *pc, unsigned int fid, unsigned int size)
    : m_client(pc),
      m_fid(fid),
      m_size(size)
{
}

unsigned int FidStream::CreateRead(ProtocolClient *pc, unsigned int fid,
				   std::auto_ptr<util::Stream> *result)
{
    unsigned int size;
    unsigned int rc = pc->Stat(fid, &size);
    if (rc != 0)
	return rc;
    result->reset(new FidStream(pc, fid, size));
    return 0;
}

unsigned int FidStream::CreateWrite(ProtocolClient *pc, unsigned int fid,
				    std::auto_ptr<util::Stream> *result)
{
    result->reset(new FidStream(pc, fid, 0));
    return 0;
}

uint64_t FidStream::GetLength()
{
    return m_size;
}

unsigned int FidStream::SetLength(uint64_t sz)
{
    unsigned int rc = m_client->Prepare(m_fid, (uint32_t)sz);
    if (rc)
	return rc;

    m_size = (unsigned int)sz;
//    TRACE "FS calls SFW\n";
    return m_client->StartFastWrite(m_fid, m_size);
}

unsigned int FidStream::ReadAt(void *buffer, uint64_t pos, size_t len, 
			       size_t *pread)
{
    uint32_t nread;
    unsigned int rc = m_client->Read(m_fid, (uint32_t)pos, (uint32_t)len,
				     buffer, &nread);
    if (rc == 0)
	*pread = nread;
    return rc;
}

unsigned int FidStream::WriteAt(const void *buffer, uint64_t pos, size_t len,
				size_t *pwritten)
{
    uint32_t nwritten;
    unsigned int rc = m_client->Write(m_fid, (uint32_t)pos, (uint32_t)len,
				      buffer, &nwritten);
    if (rc == 0)
	*pwritten = nwritten;
    return rc;
}

} // namespace empeg
