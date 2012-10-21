#ifndef LIBISAM_FORMAT_H
#define LIBISAM_FORMAT_H 1

#include <stdint.h>

namespace isam {

struct SuperBlock
{
    uint32_t format;
    uint32_t version;
    uint32_t root_data_page; 
    uint32_t bogus_shutdown;
};

enum { FORMAT = 0x15a3d00d };
enum { VERSION = 1 };

struct DataBlock
{
    uint32_t type;
    uint32_t count;
};

#if 0
struct FanoutBlock
{
    uint32_t type;
    uint32_t parent;
    uint32_t common_key_bytes;
    unsigned char common_key[1024-256-12 - 13];
    unsigned char direct_nil_value_length;
    unsigned char direct_nil_value[12];

    /** Interpretation of direct_values entry for this key byte
     *
     * b7..b4: further common key bytes (15 = no entry)
     * b3..b0: value bytes (14 = pageno, 15 = pageno of further key)
     */
    unsigned char direct_value_lengths[256];
    
    /** Values for each possible key byte.
     *
     * Each twelve-byte entry contains either:
     *   - up to eight bytes of further common key, padded to eight bytes
     *     and followed by a page number
     *   - or up to twelve bytes shared between further common key, and direct
     *     value.
     */
    unsigned char direct_values[256*12];
};
#endif

enum DataBlockType {
    NONE,
    DIRECT_KEY,
    KEY_FANOUT_not_used,
    VALUE,
    SUPERBLOCK = FORMAT
};

} // namespace isam

#endif
