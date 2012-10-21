#ifndef LIBEMPEG_PROTOCOL_H
#define LIBEMPEG_PROTOCOL_H

#undef DELETE
#include <stdint.h>

namespace empeg {

struct PacketHeader
{
    uint16_t datasize; // Not including sync byte, header, or CRC
    uint8_t  opcode;
    uint8_t  type;
    uint32_t id;
};

/** Values for PacketHeader.opcode */
enum {
    PING = 0,
    QUIT,
    MOUNT,
    NO_OPCODE,      // opcode=3 unused
    WRITE_FID,

    READ_FID,
    PREPARE,
    STAT,
    DELETE,
    REBUILD,

    FSCK,
    STATFS,
    COMMAND,
    GRAB_SCREEN,
    BEGIN_SESSION,

    SESSION_HEARTBEAT,
    END_SESSION,
    COMPLETE
};

/** Values for PacketHeader.type */
enum {
    REQUEST = 0,
    RESPONSE,
    PROGRESS
};

/** Values for CommandRequest.command */
enum {
    RESTART = 0,
    LOCK_UI,
    SLUMBER,
    SET_FID,
    TRANSPORT, // Play/pause
    BUILD_FID_LIST,
    PLAY_FID_LIST
};

/** Values for CommandRequest.param0 if command=RESTART */
enum {
    RESTART_EXIT = 0,
    RESTART_PLAYER,
    RESTART_KERNEL
};

/** Values for CommandRequest.param0 if command=TRANSPORT */
enum {
    TRANSPORT_PAUSE = 0,
    TRANSPORT_PLAY
};

struct ProgressReply
{
    PacketHeader header;
    uint32_t new_timeout;
    uint32_t stage_num;
    uint32_t stage_denom;
    uint32_t num;
    uint32_t denom;
    char details[64];
};

struct PingRequest
{
    PacketHeader header;
};

struct PingReply
{
    PacketHeader header;
    uint16_t version_minor;
    uint16_t version_major;
};

struct StatRequest
{
    PacketHeader header;
    uint32_t fid;
};

struct StatReply
{
    PacketHeader header;
    uint32_t status;
    uint32_t fid;
    uint32_t size; // sic, not uint64_t
};

struct ReadRequest
{
    PacketHeader header;
    uint32_t fid;
    uint32_t offset;
    uint32_t size;
};

struct ReadReply
{
    PacketHeader header;
    uint32_t status;
    uint32_t fid;
    uint32_t offset;
    uint32_t nread;
    // followed by the data itself
    unsigned char data[4];
};

struct MountRequest
{
    PacketHeader header;
    uint32_t partition;  // Not used, always 0 (does both partitions)
    uint32_t mode;       // 0=ro 1=rw
};

struct MountReply
{
    PacketHeader header;
    uint32_t status;
};

struct PrepareRequest
{
    PacketHeader header;
    uint32_t fid;
    uint32_t size; // sic, not uint64_t
};

struct PrepareReply
{
    PacketHeader header;
    uint32_t status;
    uint32_t fid;
    uint32_t offset;
    uint32_t size;
};

struct DeleteRequest
{
    PacketHeader header;
    uint32_t fid;
    uint32_t mask;  // b0 set -> delete *0 file, b1 set -> delete *1 file, etc
};

struct DeleteReply
{
    PacketHeader header;
    uint32_t status;
};

struct WriteRequest
{
    PacketHeader header;
    uint32_t fid;
    uint32_t offset;
    uint32_t size;
    // followed by the data itself
    unsigned char data[4];
};

struct WriteReply
{
    PacketHeader header;
    uint32_t status;
    uint32_t fid;
    uint32_t offset;
    uint32_t nwritten;
};

struct RebuildRequest
{
    PacketHeader header;
    uint32_t flags;      // Not used, always 0
};

struct RebuildReply
{
    PacketHeader header;
    uint32_t status;
};

struct CommandRequest
{
    PacketHeader header;
    uint32_t command;
    uint32_t param0;
    uint32_t param1;
    char param2[256];
};

struct CommandReply
{
    PacketHeader header;
    uint32_t status;
};


/** Packet format for reduced-overhead TCP connection.
 *
 * We were cowboys once and young.
 */
struct FastProtocolHeader
{
    uint32_t opcode;
    uint32_t fid;
    uint32_t offset;
    uint32_t size;
};

/** Values for FastProtocolHeader.opcode */
enum {
    FAST_WRITE_FID = 0
};


} // namespace empeg

#endif
