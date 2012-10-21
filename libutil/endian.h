#ifndef LIBUTIL_ENDIAN_H
#define LIBUTIL_ENDIAN_H 1

#include "config.h"
#include <stdint.h>

inline uint32_t swab32(uint32_t x)
{
    return (x >> 24)
	+ ((x >> 8) & 0xFF00)
	+ ((x << 8) & 0xFF0000)
	+ (x << 24);
}

#ifdef WORDS_BIGENDIAN
inline uint32_t cpu_to_be32(uint32_t x) { return x; }
inline uint32_t be32_to_cpu(uint32_t x) { return x; }
inline uint32_t cpu_to_le32(uint32_t x) { return swab32(x); }
inline uint32_t le32_to_cpu(uint32_t x) { return swab32(x); }
#else
inline uint32_t cpu_to_be32(uint32_t x) { return swab32(x); }
inline uint32_t be32_to_cpu(uint32_t x) { return swab32(x); }
inline uint32_t cpu_to_le32(uint32_t x) { return x; }
inline uint32_t le32_to_cpu(uint32_t x) { return x; }
#endif

#endif
