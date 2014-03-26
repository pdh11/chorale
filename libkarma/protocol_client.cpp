#include "protocol_client.h"
#include "protocol.h"
#include "libutil/trace.h"

namespace karma {

template <class REQUEST, class REPLY>
unsigned ProtocolClient::Transaction(const REQUEST *request, REPLY *reply)
{
    unsigned rc = m_socket.WriteAll(request, sizeof(REQUEST));
    if (rc) {
        return rc;
    }

    PacketHeader h;
    for (;;) {
        printf("reading\n");
        rc = m_socket.ReadAll(&h, sizeof(h));
        if (rc) {
            return rc;
        }
        if (h.opcode == NAK) {
            printf("got nak\n");
            return EINVAL;
        } else if (h.opcode == BUSY) {
            BusyReply br;
            rc = m_socket.ReadAll(&br.header + 1, sizeof(BusyReply)-sizeof(h));
            if (rc) {
                return rc;
            }
            printf("busy %u/%u\n", br.step, br.nSteps);
            continue;
        }
        else if (h.opcode != request->header.opcode) {
            printf("bad reply, expected %u got %u\n", h.opcode, 
                   request->header.opcode);
            return EINVAL;
        }
        return m_socket.ReadAll(&reply->header + 1, sizeof(REPLY) - sizeof(h));
    }
}

ProtocolClient::ProtocolClient()
{
}

unsigned ProtocolClient::Init(util::IPEndPoint ipe)
{
    m_endpoint = ipe;
    unsigned rc = m_socket.Connect(ipe);
    if (rc) {
        TRACE << "Can't connect: " << rc << "\n";
        return rc;
    }

    uint32_t major = 2, minor = 0;
    return GetProtocolVersion(&major, &minor);
}

unsigned ProtocolClient::GetProtocolVersion(uint32_t *major, uint32_t *minor)
{
    GetVersionRequest req = { { MAGIC, GET_VERSION }, *major, *minor };
    GetVersionReply reply;

    unsigned rc = Transaction(&req, &reply);
    if (rc)
        return rc;

    printf("version %u.%u\n", reply.major, reply.minor);

    return 0;
}

unsigned ProtocolClient::GetAllFileDetails(util::Stream *target)
{
    GetAllFileDetailsRequest req = { { MAGIC, GET_ALL_FILE_DETAILS } };
    GetAllFileDetailsReply reply;

    unsigned rc = Transaction(&req, &reply);
    if (rc)
        return rc;

    printf("status %u\n", reply.status);
    if (reply.status) {
        return reply.status;
    }

    unsigned char buffer[1024];

    for (;;) {
        size_t nread;
        rc = m_socket.Read(buffer, sizeof(buffer), &nread);
        if (rc) {
            return rc;
        }

        if (!nread) {
            printf("socket empty in gafd\n");
            return EIO;
        }

        rc = target->WriteAll(buffer, nread);
        if (rc) {
            return rc;
        }
        if (buffer[nread-1] == '\0') {
            return 0;
        }
    }
}

unsigned ProtocolClient::RequestIOLock(bool write)
{
    RequestIOLockRequest req = { { MAGIC, REQUEST_IO_LOCK }, write ? 1u : 0u };
    RequestIOLockReply reply;

    unsigned rc = Transaction(&req, &reply);
    if (rc)
        return rc;

    printf("status %u\n", reply.status);

    return reply.status;
}

} // namespace karma
