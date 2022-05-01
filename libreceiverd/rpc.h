#ifndef LIBRECEIVERD_RPC_H
#define LIBRECEIVERD_RPC_H 1

#include "libutil/socket.h"
#include "libutil/task.h"

namespace util { class Scheduler; }
namespace util { class IPFilter; }

namespace receiverd {

namespace rpc {

/** Structure of an RPC call buffer (names are from RFC1057)
 */
struct Call
{
    uint32_t xid;
    uint32_t msg_type;
    uint32_t rpcvers;
    uint32_t prog;
    uint32_t vers;
    uint32_t proc;
    uint32_t cred_type;
    uint32_t cred_len;
    uint32_t verf_type;
    uint32_t verf_len;
    /* ... followed by per-call parameters */
};

struct ReplySuccess
{
    uint32_t xid;
    uint32_t msg_type;
    uint32_t reply_stat;
    uint32_t verf_type;
    uint32_t verf_len;
    uint32_t accept_stat;
    /* ... followed by per-call results */
};

union BufferPtr {
    Call *call;
    ReplySuccess *success;
    unsigned char *raw;
};

enum msg_type {
    CALL = 0,
    REPLY = 1
};

enum rpcvers {
    RPCVERS = 2
};

enum reply_stat {
    MSG_ACCEPTED = 0,
    MSG_DENIED = 1
};

enum accept_stat {
    SUCCESS = 0
};

} // namespace rpc


/** Server for Sun (ONC) RPC, as needed by NFS.
 */
class RPCServer: public util::Task
{
    uint32_t m_program_number;
    uint32_t m_version;
//    util::Scheduler *m_poller;
    util::IPFilter *m_filter;
    util::DatagramSocket m_socket;
    enum { BUFSIZE = 9000 };
    static unsigned char sm_buf[BUFSIZE];

    unsigned int Run() override;
    
    typedef util::CountedPointer<RPCServer> RPCServerPtr;

public:
    RPCServer(uint32_t program_number, uint32_t version, 
	      util::Scheduler *poller, 
	      util::IPFilter *filter);
    virtual ~RPCServer() {}

    unsigned short GetPort();

    virtual unsigned int OnRPC(uint32_t proc, const void *args,
			       size_t argslen,
			       void *reply, size_t *replylen) = 0;
};

std::string String(const uint32_t *lenptr, size_t maxlen);

} // namespace receiverd

#endif
