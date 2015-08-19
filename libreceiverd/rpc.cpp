/* libreceiverd/rpc.cpp */

#include "rpc.h"
#include "libutil/trace.h"
#include "libutil/endian.h"
#include "libutil/scheduler.h"
#include "libutil/bind.h"
#include "libutil/counted_pointer.h"
#include "libutil/ip_filter.h"

#undef IN

namespace receiverd {

unsigned char RPCServer::sm_buf[9000];

RPCServer::RPCServer(uint32_t program_number, uint32_t version,
		     util::Scheduler *poller, util::IPFilter *filter)
    : m_program_number(program_number),
      m_version(version),
      m_poller(poller),
      m_filter(filter)
{
    m_socket.SetNonBlocking(true);
    util::IPEndPoint ipe = { util::IPAddress::ANY, 0 };
    m_socket.Bind(ipe);

    poller->WaitForReadable(
	util::Bind(RPCServerPtr(this)).To<&RPCServer::Run>(),
	m_socket.GetHandle(), false);
}

unsigned short RPCServer::GetPort()
{
    util::IPEndPoint ipe = m_socket.GetLocalEndPoint();
    return ipe.port;
}

unsigned int RPCServer::Run()
{
    for (;;)
    {
	size_t nread;
	util::IPEndPoint wasfrom;
	unsigned int rc = m_socket.Read(sm_buf, BUFSIZE, &nread, &wasfrom,
					NULL);
	if (rc != 0)
	    return rc;
	if (nread == 0)
	    return rc;

	if (m_filter
	    && m_filter->CheckAccess(wasfrom.addr) == util::IPFilter::DENY)
	    continue;

	if (nread < sizeof(rpc::Call))
	    continue;

	rpc::BufferPtr buf;
	buf.raw = sm_buf;

//	TRACE << "RPC xid=" << buf.call->xid
//	      << " prog=" << cpu_to_be32(buf.call->prog)
//	      << " proc=" << cpu_to_be32(buf.call->proc)
//	      << "; sz=" << nread << "\n";

	if (buf.call->msg_type != cpu_to_be32(rpc::CALL))
	    continue;

	buf.success->msg_type = cpu_to_be32(rpc::REPLY);

	if (buf.call->rpcvers != cpu_to_be32(rpc::RPCVERS))
	{
	    TRACE << "Wrong RPC version\n";
	    continue;
	}

	if (buf.call->prog != cpu_to_be32(m_program_number))
	{
	    TRACE << "Wrong program number\n";
	    continue;
	}

	if (buf.call->vers != cpu_to_be32(m_version))
	{
	    TRACE << "Wrong program version\n";
	    continue;
	}

	uint32_t *verf_ptr = &buf.call->verf_type;
	uint32_t cred_len = cpu_to_be32(buf.call->cred_len);
	if (cred_len > 1000) // Sanity check
	    continue;

	verf_ptr += (cred_len / 4);
	uint32_t *args_ptr = verf_ptr + 2;
	uint32_t verf_len = cpu_to_be32(verf_ptr[1]);
	if (verf_len > 1000)
	    continue;

	args_ptr += (verf_len / 4);

	size_t header_size = (unsigned char*)args_ptr - buf.raw;
	if (header_size > nread)
	    continue;

	size_t tosend;
	rc = OnRPC(cpu_to_be32(buf.call->proc),
		   args_ptr,
		   nread - header_size,
		   buf.success+1,
		   &tosend);
	if (rc == 0)
	{
	    buf.success->reply_stat = cpu_to_be32(rpc::MSG_ACCEPTED);
	    buf.success->verf_type = 0;
	    buf.success->verf_len = 0;
	    buf.success->accept_stat = cpu_to_be32(rpc::SUCCESS);
	    m_socket.Write(buf.raw, sizeof(rpc::ReplySuccess) + tosend,
			   wasfrom);
	}
    }
}

std::string String(const uint32_t *lenptr, size_t maxlen)
{
    size_t len = be32_to_cpu(*lenptr);
    if (len > maxlen)
	return "";
    const char *s = (const char*)(lenptr+1);
    return std::string(s, s+len);
}

} // namespace receiverd
