#include "file_stream.h"
#include "trace.h"
#include "stream_test.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace util {

unsigned FileStream::CreateTemporary(const char *filename, FileStreamPtr *fsp)
{
    int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0664);
    if (fd < 0)
	return (unsigned)errno;

    unlink(filename);
    *fsp = FileStreamPtr(new FileStream(fd));
    return 0;
}

unsigned FileStream::Create(const char *filename, unsigned int flags,
			    FileStreamPtr *fsp)
{
    int fd = open(filename, (int)flags, 0664);
    if (fd < 0)
	return (unsigned)errno;

    *fsp = FileStreamPtr(new FileStream(fd));
    return 0;
}

FileStream::FileStream(int fd)
    : m_fd(fd)
{
}

FileStream::~FileStream()
{
//    TRACE << "~FileStream\n";
    close(m_fd);
}

unsigned FileStream::Read(void *buffer, size_t len, size_t *pread)
{
    ssize_t rc = ::read(m_fd, buffer, len);
    if (rc < 0)
    {
	TRACE << "FS::Read failed len=" << len << "\n";
	*pread = 0;
	return (unsigned)errno;
    }
    *pread = (size_t)rc;
    return 0;
}

unsigned FileStream::Write(const void *buffer, size_t len, size_t *pwrote)
{
    ssize_t rc = ::write(m_fd, buffer, len);
    if (rc < 0)
    {
	*pwrote = 0;
	return (unsigned)errno;
    }
//    TRACE << "fs wrote " << rc << "/" << len << "\n";
    *pwrote = (size_t)rc;
    return 0;
}

void FileStream::Seek(pos64 pos)
{
    lseek(m_fd, (off_t)pos, SEEK_SET);
}

SeekableStream::pos64 FileStream::Tell()
{
    return (pos64)lseek(m_fd, 0, SEEK_CUR);
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

} // namespace util

#ifdef TEST

int main()
{
    util::FileStreamPtr msp;

    unsigned int rc = util::FileStream::CreateTemporary("test.tmp", &msp);
    assert(rc == 0);

    TestSeekableStream(msp);

    return 0;
}

#endif
