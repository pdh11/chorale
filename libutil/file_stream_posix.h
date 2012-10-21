#ifndef FILE_STREAM_POSIX_H
#define FILE_STREAM_POSIX_H

#include "file_stream.h"
#include <boost/noncopyable.hpp>

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
    unsigned GetStreamFlags() const { return READABLE|WRITABLE|SEEKABLE|POLLABLE; }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote);
    uint64_t GetLength();
    unsigned SetLength(uint64_t);

    int GetHandle() { return m_fd; }
};

} // namespace posix

} // namespace util

#endif
