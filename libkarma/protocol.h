#ifndef LIB_KARMA_PROTOCOL_H
#define LIB_KARMA_PROTOCOL_H 1

#include <stdint.h>

namespace karma {

struct PacketHeader
{
    uint32_t magic;
    uint32_t opcode;
};

/** For PacketHeader.magic
 */
enum { MAGIC = 0x8DC56952 };

/** For PacketHeader.opcode
 */
enum {
    GET_VERSION,
    NAK,
    BUSY,
    GET_CHALLENGE,

    AUTHENTICATE,
    GET_DEVICE_DETAILS,
    GET_STORAGE_DETAILS,
    GET_DEVICE_SETTINGS,

    UPDATE_DEVICE_SETTINGS,
    REQUEST_IO_LOCK,
    RELEASE_IO_LOCK,
    PREPARE,

    WRITE_FID,
    GET_ALL_FILE_DETAILS,
    GET_FILE_DETAILS,
    UPDATE_FILE_DETAILS,

    READ_FID,
    DELETE,
    FORMAT,
    COMPLETE,

    DEVICE_OPERATION
};

struct GetVersionRequest
{
    PacketHeader header;
    uint32_t major;
    uint32_t minor;
};

struct GetVersionReply
{
    PacketHeader header;
    uint32_t major;
    uint32_t minor;
};

struct NakReply
{
    PacketHeader header;
};

struct BusyReply
{
    PacketHeader header;
    uint32_t step;
    uint32_t nSteps;
};

struct GetChallengeRequest
{
    PacketHeader header;
};

struct GetChallengeReply
{
    PacketHeader header;
    unsigned char challenge[16];
};

struct GetAllFileDetailsRequest
{
    PacketHeader header;
};

struct GetAllFileDetailsReply
{
    PacketHeader header;
    uint32_t status;
};

struct RequestIOLockRequest
{
    PacketHeader header;
    uint32_t write;
};

struct RequestIOLockReply
{
    PacketHeader header;
    uint32_t status;
};

} // namespace karma

#endif
