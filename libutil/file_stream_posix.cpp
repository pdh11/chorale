#include "config.h"
#include "file_stream_posix.h"
#include "trace.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace util {

namespace posix {

FileStream::FileStream()
    : m_fd(-1), m_mode(0)
{
}

unsigned FileStream::Open(const char *filename, unsigned int mode)
{
    unsigned flags = 0;

    switch (mode & TYPE_MASK)
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
    m_mode = mode;

    if ((mode & TYPE_MASK) == TEMP)
	unlink(filename);

#if HAVE_POSIX_FADVISE
    if (mode & SEQUENTIAL)
	posix_fadvise(m_fd, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

    return 0;
}

FileStream::~FileStream()
{
//    TRACE << "~FileStream\n";
    if (m_fd >= 0)
	close(m_fd);
}

unsigned FileStream::ReadAt(void *buffer, uint64_t pos, size_t len, 
			    size_t *pnread)
{
#if HAVE_PREAD64
    ssize_t rc = ::pread64(m_fd, buffer, len, pos);
#else
    ssize_t rc = ::pread(m_fd, buffer, len, pos);
#endif
    if (rc < 0)
    {
	TRACE << "FS::Read failed len=" << len << "\n";
	*pnread = 0;
	return (unsigned)errno;
    }
    if (rc == 0)
    {
	TRACE << "FS::Read EOF at pos=" << pos << "\n";
    }
    *pnread = (size_t)rc;
    return 0;
}

unsigned FileStream::WriteAt(const void *buffer, uint64_t pos, size_t len, 
			     size_t *pwrote)
{
#if HAVE_PWRITE64
    ssize_t rc = ::pwrite64(m_fd, buffer, len, pos);
#else
    ssize_t rc = ::pwrite(m_fd, buffer, len, pos);
#endif
    if (rc < 0)
    {
	*pwrote = 0;
	TRACE << "FS::Write failed " << errno << "\n";
	return (unsigned)errno;
    }

#if HAVE_SYNC_FILE_RANGE
    const uint64_t PAGE_SIZE = 4096; // A plausible guess

    if ((m_mode & SEQUENTIAL) && (pos+rc >= PAGE_SIZE))
	sync_file_range(m_fd, 0, (pos+rc) & ~(PAGE_SIZE-1), 
			SYNC_FILE_RANGE_WRITE);
#endif

//    TRACE << "fs wrote " << rc << "/" << len << "\n";
    *pwrote = (size_t)rc;
    return 0;
}

uint64_t FileStream::GetLength()
{
    struct stat st;
    int rc = fstat(m_fd, &st);
    if (rc < 0)
	return 0;
    return (uint64_t)st.st_size;
}

unsigned FileStream::SetLength(uint64_t len)
{
    int rc = ftruncate(m_fd, (off_t)len);
    if (rc<0)
	return (unsigned)errno;
    if (Tell() > len)
	Seek(len);
    posix_fallocate(m_fd, 0, len);
    return 0;
}

} // namespace posix

} // namespace util

#ifdef TEST

# include "stream_test.h"

int main()
{
    std::unique_ptr<util::Stream> msp;

    unsigned int rc = util::OpenFileStream("test.tmp", util::TEMP, &msp);
    assert(rc == 0);

    TestSeekableStream(msp.get());

    return 0;
}

#endif
