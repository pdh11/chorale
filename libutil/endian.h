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

inline void write_be16(unsigned char *dest, uint16_t val)
{
    dest[0] = (unsigned char)(val>>8);
    dest[1] = (unsigned char)val;
}

inline void write_be24(unsigned char *dest, uint32_t val)
{
    dest[0] = (unsigned char)(val>>16);
    dest[1] = (unsigned char)(val>>8);
    dest[2] = (unsigned char)val;
}

inline void write_be32(unsigned char *dest, uint32_t val)
{
    dest[0] = (unsigned char)(val>>24);
    dest[1] = (unsigned char)(val>>16);
    dest[2] = (unsigned char)(val>>8);
    dest[3] = (unsigned char)val;
}

inline uint16_t read_be16(const unsigned char *src)
{
    return (uint16_t)((*src)*256 + src[1]);
}

inline uint32_t read_be24(const unsigned char *src)
{
    return (src[0] << 16) + (src[1] << 8) + src[2];
}

inline uint32_t read_be32(const unsigned char *src)
{
    return (src[0] << 24) + (src[1] << 16) + (src[2] << 8) + src[3];
}

inline void write_le32(unsigned char *dest, uint32_t val)
{
    dest[3] = (unsigned char)(val>>24);
    dest[2] = (unsigned char)(val>>16);
    dest[1] = (unsigned char)(val>>8);
    dest[0] = (unsigned char)val;
}

#endif
