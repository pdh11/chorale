#include "protocol_client.h"
#include "protocol.h"
#include "crc16.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#include <string.h>

namespace empeg {

ProtocolClient::ProtocolClient()
    : m_aligned_buffer(new uint64_t[(MAX_PAYLOAD + 8 + 16 + 2 + 1 + 7)/8]),
      m_buffer((unsigned char*)m_aligned_buffer),
      m_packet_id(0),
      m_fast(0)
{
}

ProtocolClient::~ProtocolClient()
{
    delete[] m_buffer;
}

void ProtocolClient::GetNextPacketId()
{
    /* For what are presumably historical reasons, Emptool only uses ids where
     * each octet has its top bit set. So, so do we.
     */
    m_packet_id &= 0x7F7F7F7F;
    ++m_packet_id;
    if (m_packet_id & 0x80)
	m_packet_id += 0x80;
    if (m_packet_id & 0x8000)
	m_packet_id += 0x8000;
    if (m_packet_id & 0x800000)
	m_packet_id += 0x800000;
    m_packet_id |= 0x80808080;
}

unsigned int ProtocolClient::Init(util::IPAddress ip)
{
    m_address = ip;
    util::IPEndPoint ipe = { m_address, PROTOCOL_PORT };
    unsigned int rc = m_socket.Connect(ipe);
    if (rc != 0)
    {
	TRACE << "Can't connect: " << rc << "\n";
	return rc;
    }

    ipe.port = PROTOCOL_FAST_PORT;
    rc = m_fastsocket.Connect(ipe);
    if (rc)
    {
	TRACE << "Can't connect fast socket: " << rc << "\n";
//	return rc;
	m_fastsocket.Close();
    }

    uint16_t major, minor;
    rc = Ping(&minor, &major);

//    TRACE << "Protocol version " << major << ":" << minor << "\n";

    return rc;
}

unsigned int ProtocolClient::Transaction()
{
    PacketHeader *header = (PacketHeader*)m_aligned_buffer;

    GetNextPacketId();
    header->type = REQUEST;
    header->id = m_packet_id;
    uint16_t crc = CRC16(m_buffer+2, header->datasize + 2 + 4);

    m_buffer[header->datasize + 8] = (unsigned char)crc;
    m_buffer[header->datasize + 8 + 1] = (unsigned char)(crc >> 8);

    unsigned char syncbyte = 2;

    TRACE << "Sending packet, len=" << header->datasize << " op="
	  << header->opcode << " type=" << header->type << "\n";

    m_socket.SetCork(true);
    unsigned rc = m_socket.WriteAll(&syncbyte, 1);
    if (rc != 0)
	return rc;
    rc = m_socket.WriteAll(m_buffer, header->datasize + 10);
    m_socket.SetCork(false);
    if (rc != 0)
	return rc;
    
    while (1)
    {
	rc = m_socket.ReadAll(&syncbyte, 1);

	if (rc != 0)
	{
	    TRACE << "No sync byte (" << rc << ")\n";
	    return rc;
	}

	if (syncbyte != 2)
	{
	    TRACE << "Wrong sync byte\n";
	    /** @todo Skip until we find one? */
	    return EINVAL;
	}

	rc = m_socket.ReadAll(m_buffer, sizeof(PacketHeader));
	if (rc != 0)
	{
	    TRACE << "No header (" << rc << ")\n";
	    return rc;
	}
	
//	TRACE << "Incoming packet, len=" << header->datasize << " op="
//	      << header->opcode << " type=" << header->type << "\n";

	rc = m_socket.ReadAll(m_buffer+sizeof(PacketHeader),
			       header->datasize + 2);
	if (rc != 0)
	{
	    TRACE << "No packet (" << rc << ")\n";
	    return rc;
	}

	/* Because (unlike Emptool) we're only using TCP, not serial or USB,
	 * we needn't bother checking the CRC.
	 */
	switch (header->type)
	{
	case PROGRESS:
	{
	    ProgressReply *progress = (ProgressReply*)m_aligned_buffer;
//	    TRACE << "Progress packet " << progress->stage_num << "/"
//		  << progress->stage_denom << ", " << progress->num
//		  << "/" << progress->denom << "\n";
	    Fire(&ProtocolObserver::OnProgress,
		 progress->stage_num,
		 progress->stage_denom,
		 progress->num,
		 progress->denom);

	    continue;
	}
	case RESPONSE:
	    return 0;
	default:
	    TRACE << "Unknown packet type\n";
	    return EINVAL;
	}
    }
}

unsigned int ProtocolClient::Ping(uint16_t *minor, uint16_t *major)
{
    util::Mutex::Lock lock(m_mutex);

    PingRequest *req = (PingRequest*)m_aligned_buffer;
    req->header.datasize = 0;
    req->header.opcode = PING;
    unsigned int rc = Transaction();

    if (rc == 0)
    {
	PingReply *reply = (PingReply*)m_aligned_buffer;
	if (minor)
	    *minor = reply->version_minor;
	if (major)
	    *major = reply->version_major;
    }
 
    return rc;
}

unsigned int ProtocolClient::Stat(uint32_t fid, uint32_t *size)
{
    util::Mutex::Lock lock(m_mutex);

    StatRequest *req = (StatRequest*)m_aligned_buffer;
    req->header.datasize = 4;
    req->header.opcode = STAT;
    req->fid = fid;
    unsigned int rc = Transaction();

    if (rc != 0)
	return rc;

    StatReply *reply = (StatReply*)m_aligned_buffer;

//    TRACE << "status " << reply->status << " id " << reply->fid << " size "
//	  << reply->size << "\n";

    if (reply->status)
    {
	TRACE << "Stat(" << fid << ") returned error " << reply->status << "\n";
	return ENOENT;
    }

    if (size)
	*size = reply->size;

    return 0;
}

unsigned int ProtocolClient::Read(uint32_t fid, uint32_t offset, uint32_t size,
				  void *buffer, uint32_t *nread)
{
    util::Mutex::Lock lock(m_mutex);

    if (size > MAX_PAYLOAD)
	size = MAX_PAYLOAD;

    ReadRequest *req = (ReadRequest*)m_aligned_buffer;
    req->header.datasize = sizeof(ReadRequest) - sizeof(PacketHeader);
    req->header.opcode = READ_FID;
    req->fid = fid;
    req->offset = offset;
    req->size = size;
    unsigned int rc = Transaction();
    if (rc != 0)
	return rc;

    ReadReply *reply = (ReadReply*)m_aligned_buffer;
    
    if (reply->status)
    {
	TRACE << "Read returned error " << reply->status << "\n";
	return ENOENT;
    }

    uint32_t actualsz = std::min(size, reply->nread);

    memcpy(buffer, reply->data, actualsz);
    *nread = actualsz;

    return 0;
}

unsigned int ProtocolClient::EnableWrite(bool whether)
{
    util::Mutex::Lock lock(m_mutex);
    MountRequest *req = (MountRequest*)m_aligned_buffer;
    req->header.datasize = sizeof(MountRequest) - sizeof(PacketHeader);
    req->header.opcode = MOUNT;
    req->partition = 0;
    req->mode = whether ? 1 : 0;
    unsigned int rc = Transaction();
    if (rc != 0)
	return rc;
    
    MountReply *reply = (MountReply*)m_aligned_buffer;
    if (reply->status)
    {
	TRACE << "Mount returned error " << reply->status << "\n";
	return EIO;
    }
    return 0;
}

unsigned int ProtocolClient::RebuildDatabase()
{
    util::Mutex::Lock lock(m_mutex);
    RebuildRequest *req = (RebuildRequest*)m_aligned_buffer;
    req->header.datasize = sizeof(RebuildRequest) - sizeof(PacketHeader);
    req->header.opcode = REBUILD;
    req->flags = 0;
    unsigned int rc = Transaction();
    if (rc != 0)
	return rc;
    
    RebuildReply *reply = (RebuildReply*)m_aligned_buffer;
    if (reply->status)
    {
	TRACE << "Rebuild returned error " << reply->status << "\n";
	return EIO;
    }
    return 0;
}

unsigned int ProtocolClient::Delete(uint32_t fid, uint16_t mask)
{
    util::Mutex::Lock lock(m_mutex);
    DeleteRequest *req = (DeleteRequest*)m_aligned_buffer;
    req->header.datasize = sizeof(DeleteRequest) - sizeof(PacketHeader);
    req->header.opcode = DELETE;
    req->fid = fid;
    req->mask = mask;
    unsigned int rc = Transaction();
    if (rc != 0)
	return rc;
    
    DeleteReply *reply = (DeleteReply*)m_aligned_buffer;
    if (reply->status)
    {
	TRACE << "Delete returned error " << reply->status << "\n";
	return EPERM;
    }
    return 0;
}

unsigned int ProtocolClient::Prepare(uint32_t fid, uint32_t size)
{
    util::Mutex::Lock lock(m_mutex);
    PrepareRequest *req = (PrepareRequest*)m_aligned_buffer;
    req->header.datasize = sizeof(PrepareRequest) - sizeof(PacketHeader);
    req->header.opcode = PREPARE;
    req->fid = fid;
    req->size = size;
//    TRACE << "Prepare(" << fid << ", " << size << ")\n";
    unsigned int rc = Transaction();
    if (rc != 0)
	return rc;
    
    PrepareReply *reply = (PrepareReply*)m_aligned_buffer;
    if (reply->status)
    {
	TRACE << "Prepare returned error " << reply->status << "\n";
	return EPERM;
    }
//    TRACE << "Prepare returned fid " << reply->fid << " offset "
//	  << reply->offset << " size " << reply->size << "\n";

    return 0;
}

unsigned int ProtocolClient::StartFastWrite(uint32_t fid, uint32_t size)
{
    FastProtocolHeader fph;
    fph.opcode = FAST_WRITE_FID;
    fph.fid = fid;
    fph.offset = 0;
    fph.size = size;

    if (!m_fastsocket.IsOpen())
    {
	m_fastsocket.Open();
	util::IPEndPoint ipe = { m_address, PROTOCOL_FAST_PORT };
	unsigned int rc = m_fastsocket.Connect(ipe);
	if (rc != 0)
	{
	    TRACE << "Can't connect fast socket\n";
	    m_fastsocket.Close();
	    return rc;
	}
    }
    unsigned int rc = m_fastsocket.WriteAll(&fph, sizeof(fph));
    m_fast = size;
//    TRACE << "Starting fast write of " << size << " bytes to FID " << fid
//	  << "\n";
    return rc;
}

unsigned int ProtocolClient::Write(uint32_t fid, uint32_t offset,
				   uint32_t size, const void *buffer, 
				   uint32_t *nwritten)
{
    if (!size)
	return 0;

    util::Mutex::Lock lock(m_mutex);

    if (!m_fast)
    {
	TRACE << "Warning, can't write without fast connection\n";
	return EOPNOTSUPP;
    }

    if (m_fast)
    {
//	TRACE << "m_fast now " << m_fast << "\n";
//	TRACE << "Trying to fastwrite FID " << fid << " " << size 
//	      << " bytes at offset " << offset << "\n";

	unsigned int rc = m_fastsocket.WriteAll(buffer, size);
	if (rc)
	{
	    TRACE << "Fast protocol failed: " << rc << "\n";
	    m_fastsocket.Close();
	}
	else
	    *nwritten = size;
	m_fast -= size;
	if (!m_fast)
	{
	    /* I hate things like this. Almost certainly it represents
	     * a bug at the Empeg end, one that never showed up
	     * against the slow PCs we had back in the day. But this
	     * code is 100% reliable with the sleeping, and 0%
	     * reliable without it, so here it must stay.
	     */
	    sleep(1);
	}
	return rc;
    }

    WriteRequest *req = (WriteRequest*)m_aligned_buffer;
    if (size > MAX_PAYLOAD)
	size = MAX_PAYLOAD;

    req->header.datasize = (uint16_t)(sizeof(WriteRequest)
				      - 4
				      - sizeof(PacketHeader)
				      + size);
    req->header.opcode = WRITE_FID;
    req->fid = fid;
    req->offset = offset;
    req->size = size;
    memcpy(req->data, buffer, size);

    TRACE << "Trying to write FID " << fid << " " << size << " bytes at "
	  << offset << " ds=" << req->header.datasize << "\n";

    unsigned int rc = Transaction();
    if (rc != 0)
	return rc;

    WriteReply *reply = (WriteReply*)m_aligned_buffer;
    
    if (reply->status)
    {
	TRACE << "Write returned error " << reply->status << "\n";
	return ENOENT;
    }

    uint32_t actualsz = std::min(size, reply->nwritten);
    *nwritten = actualsz;

    return 0;
}

unsigned int ProtocolClient::SendCommand(uint32_t command, uint32_t param0,
					 uint32_t param1, const char *param2)
{
    util::Mutex::Lock lock(m_mutex);

    CommandRequest *req = (CommandRequest*)m_aligned_buffer;
    req->header.datasize = sizeof(CommandRequest) - sizeof(PacketHeader);
    req->header.opcode = COMMAND;
    req->command = command;
    req->param0 = param0;
    req->param1 = param1;
    strncpy(req->param2, param2 ? param2 : "", sizeof(req->param2));
    unsigned int rc = Transaction();
    if (rc != 0)
	return rc;
    
    CommandReply *reply = (CommandReply*)m_aligned_buffer;
    if (reply->status)
    {
	TRACE << "Command returned error " << reply->status << "\n";
	return EPERM;
    }
    return 0;
}

/** Causes the player to exit; init will then re-exec it again.
 *
 * This, of course, causes it to drop the connection.
 */
unsigned int ProtocolClient::RestartPlayer()
{
    return SendCommand(RESTART, RESTART_PLAYER);
}

unsigned int ProtocolClient::LockUI(bool locked)
{
    return SendCommand(LOCK_UI, locked ? 1 : 0);
}


        /* Higher-level operations */


unsigned int ProtocolClient::ReadFidToBuffer(uint32_t fid, uint32_t *size,
					     boost::scoped_array<char> *buffer)
{
    unsigned int sz;
    unsigned int rc = Stat(fid, &sz);
    if (rc != 0)
	return rc;

    *size = sz;

    buffer->reset(new char[sz]);

    unsigned int offset = 0;
    while (offset < sz)
    {
	unsigned int lump = std::min(sz-offset, (unsigned int)MAX_PAYLOAD);

	unsigned int nread;
	rc = Read(fid, offset, lump, buffer->get() + offset, &nread);

	if (rc != 0)
	    return rc;

	offset += nread;
    }

    return 0;
}

unsigned int ProtocolClient::ReadFidToString(uint32_t fid, std::string *dest)
{
    dest->clear();

    uint32_t sz;
    boost::scoped_array<char> buffer;
    unsigned int rc = ReadFidToBuffer(fid, &sz, &buffer);
    if (rc)
	return rc;
    *dest = std::string(buffer.get(), sz);
    return 0;
}

unsigned int ProtocolClient::WriteFidFromString(uint32_t fid, 
						const std::string& s)
{
    unsigned int rc = Prepare(fid, (uint32_t)s.length());
    if (rc)
    {
	TRACE << "Prepare failed: " << rc << "\n";
	return rc;
    }

    rc = StartFastWrite(fid, (uint32_t)s.length());
    if (rc)
    {
	TRACE << "SFW failed\n";
	return rc;
    }

    uint32_t nwritten;
    return Write(fid, 0, (uint32_t)s.length(), s.c_str(), &nwritten);
}

} // namespace empeg

#ifdef TEST

# include "discovery.h"
# include "libutil/scheduler.h"
# include "libutil/file_stream.h"
# include <boost/scoped_array.hpp>

static void ReadFidToFile(empeg::ProtocolClient *pc, uint32_t fid,
                          const char *filename)
{
    unsigned int sz;

    boost::scoped_array<char> buf;

    unsigned int rc = pc->ReadFidToBuffer(fid, &sz, &buf);

    std::unique_ptr<util::Stream> ssp;
    rc = util::OpenFileStream(filename, util::WRITE, &ssp);
    if (rc != 0)
	return;

    rc = ssp->WriteAll(buf.get(), sz);
    if (rc != 0)
	return;

    TRACE << "Wrote " << sz << " bytes to " << filename << "\n";
}

class DECallback: public empeg::Discovery::Callback
{
public:
    void OnDiscoveredEmpeg(const util::IPAddress& ip, const std::string& name)
    {
	TRACE << "Found Empeg '" << name << "' on " << ip.ToString() << "\n";

	empeg::ProtocolClient pc;
	unsigned int rc = pc.Init(ip);
	if (rc != 0)
	    return;
	unsigned int tsize, dsize, psize;
	rc = pc.Stat(empeg::FID_TAGS, &tsize);
	if (rc == 0)
	    rc = pc.Stat(empeg::FID_DATABASE, &dsize);
	if (rc == 0)
	    rc = pc.Stat(empeg::FID_PLAYLISTS, &psize);
	if (rc != 0)
	    return;

	std::string version;
	pc.ReadFidToString(empeg::FID_VERSION, &version);

	TRACE << "tags " << tsize << ", db " << dsize << ", playlists "
	      << psize << " version " << version << "\n";


	ReadFidToFile(&pc, empeg::FID_TAGS, "tags");
	ReadFidToFile(&pc, empeg::FID_DATABASE, "database3");
	ReadFidToFile(&pc, empeg::FID_PLAYLISTS, "playlists");
    }
};

int main(int, char **)
{
    util::BackgroundScheduler poller;

    empeg::Discovery disc;

    DECallback dec;

    disc.Init(&poller, &dec);

    time_t t = time(NULL);

    while ((time(NULL) - t) < 5)
    {
	poller.Poll(1000);
    }

    return 0;
}

#endif
