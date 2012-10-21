#include "file_stream_posix.h"
#include "trace.h"
#include "stream_test.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef WIN32

namespace util {

namespace posix {

FileStream::FileStream()
    : m_fd(-1)
{
}

unsigned FileStream::Open(const char *filename, FileMode mode)
{
    unsigned flags = 0;

    switch (mode)
    {
    case READ:   flags = O_RDONLY; break;
    case WRITE:  flags = O_RDWR|O_CREAT|O_TRUNC; break;
    case UPDATE: flags = O_RDWR; break;
    case TEMP:   flags = O_RDWR|O_CREAT|O_TRUNC; break;
    }

    if (m_fd >= 0)
	close(m_fd);

    m_fd = open(filename, flags, 0644);
    if (m_fd < 0)
	return (unsigned)errno;

    if (mode == TEMP)
	unlink(filename);
    return 0;
}

FileStream::~FileStream()
{
//    TRACE << "~FileStream\n";
    if (m_fd >= 0)
	close(m_fd);
}

unsigned FileStream::ReadAt(void *buffer, pos64 pos, size_t len, 
			    size_t *pread)
{
    ssize_t rc = ::pread64(m_fd, buffer, len, pos);
    if (rc < 0)
    {
	TRACE << "FS::Read failed len=" << len << "\n";
	*pread = 0;
	return (unsigned)errno;
    }
    *pread = (size_t)rc;
    return 0;
}

unsigned FileStream::WriteAt(const void *buffer, pos64 pos, size_t len, 
			     size_t *pwrote)
{
    ssize_t rc = ::pwrite64(m_fd, buffer, len, pos);
    if (rc < 0)
    {
	*pwrote = 0;
	return (unsigned)errno;
    }
//    TRACE << "fs wrote " << rc << "/" << len << "\n";
    *pwrote = (size_t)rc;
    return 0;
}

SeekableStream::pos64 FileStream::GetLength()
{
    struct stat st;
    int rc = fstat(m_fd, &st);
    if (rc < 0)
	return 0;
    return (pos64)st.st_size;
}

unsigned FileStream::SetLength(pos64 len)
{
    int rc = ftruncate(m_fd, (off_t)len);
    if (rc<0)
	return (unsigned)errno;
    if (Tell() > len)
	Seek(len);
    return 0;
}

} // namespace posix

} // namespace util

#endif // !WIN32

#ifdef TEST

int main()
{
    util::SeekableStreamPtr msp;

    unsigned int rc = util::OpenFileStream("test.tmp", util::TEMP, &msp);
    assert(rc == 0);

    TestSeekableStream(msp);

    return 0;
}

#endif
