#include "file_stream_win32.h"
#include "trace.h"
#include "utf8.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef WIN32

#include <windows.h>

namespace util {

namespace win32 {

FileStream::FileStream()
    : m_fd(INVALID_HANDLE_VALUE)
{
}

unsigned FileStream::Open(const char *filename, unsigned int mode)
{
    unsigned access = 0, disposition = 0, fanda = 0;

    switch (mode & TYPE_MASK)
    {
    case READ:
	access = GENERIC_READ;
	disposition = OPEN_EXISTING;
	fanda = FILE_ATTRIBUTE_NORMAL;
	break;
    case WRITE:
	access = GENERIC_READ|GENERIC_WRITE;
	disposition = CREATE_ALWAYS;
	fanda = FILE_ATTRIBUTE_NORMAL;
	break;
    case UPDATE:
	access = GENERIC_READ|GENERIC_WRITE;
	disposition = OPEN_ALWAYS;
	fanda = FILE_ATTRIBUTE_NORMAL;
	break;
    case TEMP:
	access = GENERIC_READ|GENERIC_WRITE;
	disposition = CREATE_ALWAYS;
	fanda = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
	break;
    }

    if (mode & SEQUENTIAL)
	fanda |= FILE_FLAG_SEQUENTIAL_SCAN;

    utf16string wfilename = UTF8ToUTF16(filename);

    m_fd = CreateFileW(wfilename.c_str(), access,
		       FILE_SHARE_READ|FILE_SHARE_WRITE,
		       NULL, disposition, fanda, NULL);

    if (m_fd == INVALID_HANDLE_VALUE)
    {
	TRACE << "Can't open file '" << wfilename << "' (" << GetLastError()
	      << ")\n";
	return ENOENT;
    }

    return 0;
}

FileStream::~FileStream()
{
//    TRACE << "~FileStream\n";
    if (m_fd != INVALID_HANDLE_VALUE)
	CloseHandle(m_fd);
}

unsigned FileStream::ReadAt(void *buffer, uint64_t pos, size_t len, 
			    size_t *pread)
{
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    ov.Offset = (uint32_t)pos;
    ov.OffsetHigh = (uint32_t)(pos >> 32);

    DWORD nread = 0;
    bool ok = ReadFile(m_fd, buffer, len, &nread, &ov);

    if (!ok)
    {
	unsigned int err = GetLastError();
	if (err != ERROR_HANDLE_EOF)
	    return err;
    }
    
    *pread = nread;
    return 0;
}

unsigned FileStream::WriteAt(const void *buffer, uint64_t pos, size_t len, 
			     size_t *pwrote)
{
    OVERLAPPED ov;
    memset(&ov, 0, sizeof(ov));
    ov.Offset = (uint32_t)pos;
    ov.OffsetHigh = (uint32_t)(pos >> 32);

    DWORD nwrote;
    bool ok = WriteFile(m_fd, buffer, len, &nwrote, &ov);

    if (!ok)
	return GetLastError();

    *pwrote = nwrote;
    return 0;
}

uint64_t FileStream::GetLength()
{
    DWORD hi;
    DWORD lo = GetFileSize(m_fd, &hi);
    if (lo == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
	return 0;
    return (((uint64_t)hi) << 32) + lo;
}

unsigned FileStream::SetLength(uint64_t len)
{
    /* Although atomic pread() and pwrite are implemented (in NT-derived
     * Windows only), atomic ftruncate() is not -- except on Vista. :(
     */
    LONG hi = (LONG)(len >> 32);
    SetFilePointer(m_fd, (DWORD)len, &hi, FILE_BEGIN);
    SetEndOfFile(m_fd);

    /* Sync-up internal idea of file pointer */
    Seek(len);
    return 0;
}

} // namespace win32

} // namespace util

#endif // WIN32

#ifdef TEST
# include "stream_test.h"

int main()
{
    std::auto_ptr<util::Stream> msp;

    unsigned int rc = util::OpenFileStream("test.tmp", util::TEMP, &msp);
    assert(rc == 0);

    TestSeekableStream(msp.get());

    return 0;
}

#endif
