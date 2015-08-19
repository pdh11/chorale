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
    unsigned GetStreamFlags() const override
    {
        return READABLE|WRITABLE|SEEKABLE;
    }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
                    size_t *pread) override;
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote) override;
    uint64_t GetLength() override;
    unsigned SetLength(uint64_t) override;
};

} // namespace win32

} // namespace util

#endif
