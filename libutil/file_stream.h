#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include "stream.h"

namespace util {

class FileStream: public SeekableStream, private boost::noncopyable
{
    int m_fd;
    explicit FileStream(int fd);
    ~FileStream();

public:
    typedef boost::intrusive_ptr<FileStream> FileStreamPtr;

    /** flags are as for open(1) */
    static unsigned Create(const char *filename, unsigned flags,
			   FileStreamPtr*) ATTRIBUTE_WARNUNUSED; 

    /** automatically deleted when no longer needed */
    static unsigned CreateTemporary(const char *filename, FileStreamPtr*)
	ATTRIBUTE_WARNUNUSED; 

    // Being a SeekableStream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    void Seek(pos64 pos);
    pos64 Tell();
    pos64 GetLength();
    unsigned SetLength(pos64);
};

typedef boost::intrusive_ptr<FileStream> FileStreamPtr;

}; // namespace util

#endif
