#ifndef FILE_STREAM_POSIX_H
#define FILE_STREAM_POSIX_H

#include "file_stream.h"

namespace util {

namespace posix {

class FileStream: public SeekableStream, private boost::noncopyable
{
    int m_fd;
    unsigned int m_mode;

public:
    FileStream();
    ~FileStream();

    unsigned Open(const char *filename, unsigned int mode);

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

} // namespace posix

} // namespace util

#endif
