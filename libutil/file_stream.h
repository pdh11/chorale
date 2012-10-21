#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include "stream.h"

namespace util {

enum FileMode {
    READ,   // Like O_RDONLY
    WRITE,  // Like O_RDWR|O_CREAT|O_TRUNC
    UPDATE, // Like O_RDWR
    TEMP    // Will be deleted on close (implies WRITE)
};

unsigned int OpenFileStream(const char *filename, FileMode,
			    SeekableStreamPtr*) ATTRIBUTE_WARNUNUSED;

} // namespace util

#endif
