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
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

} // namespace win32

} // namespace util

#endif
