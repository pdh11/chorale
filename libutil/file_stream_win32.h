#ifndef FILE_STREAM_WIN32_H
#define FILE_STREAM_WIN32_H

#include "file_stream.h"
#include <boost/noncopyable.hpp>

namespace util {

namespace win32 {

class FileStream: public SeekableStream, private boost::noncopyable
{
    void* m_fd;

public:
    FileStream();
    ~FileStream();

    unsigned Open(const char *filename, unsigned int mode);

    // Being a SeekableStream
    unsigned GetStreamFlags() const { return READABLE|WRITABLE|SEEKABLE; }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote);
    uint64_t GetLength();
    unsigned SetLength(uint64_t);
};

} // namespace win32

} // namespace util

#endif
