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
    static unsigned Create(const char *filename, unsigned int flags,
			   FileStreamPtr*) ATTRIBUTE_WARNUNUSED; 

    /** automatically deleted when no longer needed */
    static unsigned CreateTemporary(const char *filename, FileStreamPtr*)
	ATTRIBUTE_WARNUNUSED; 

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, size_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, size_t pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

typedef boost::intrusive_ptr<FileStream> FileStreamPtr;

} // namespace util

#endif
