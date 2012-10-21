#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include "stream.h"

namespace util {

enum FileMode {

    // Basic types

    READ   = 0, // Like O_RDONLY
    WRITE  = 1, // Like O_RDWR|O_CREAT|O_TRUNC
    UPDATE = 2, // Like O_RDWR
    TEMP   = 3, // Will be deleted on close (implies WRITE)

    TYPE_MASK = 3,

    // Additional flags

    SEQUENTIAL = 4
};

unsigned int OpenFileStream(const char *filename, unsigned int mode,
			    SeekableStreamPtr*) ATTRIBUTE_WARNUNUSED;

} // namespace util

#endif
