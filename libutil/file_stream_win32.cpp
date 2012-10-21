#include "file_stream_win32.h"
#include "trace.h"
#include "utf8.h"
#include "stream_test.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef WIN32

namespace util {

namespace win32 {

FileStream::FileStream()
    : m_fd(INVALID_HANDLE_VALUE)
{
}

unsigned FileStream::Open(const char *filename, FileMode mode)
{
    unsigned access = 0, disposition = 0, fanda = 0;

    switch (mode)
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

    /** @todo CreateFileW(utf8toutf16(...)...) */
    m_fd = CreateFileA(filename, access, FILE_SHARE_READ|FILE_SHARE_WRITE,
		      NULL, disposition, fanda, NULL);

    if (m_fd == INVALID_HANDLE_VALUE)
	return GetLastError();

    return 0;
}

FileStream::~FileStream()
{
//    TRACE << "~FileStream\n";
    if (m_fd != INVALID_HANDLE_VALUE)
	CloseHandle(m_fd);
}

unsigned FileStream::ReadAt(void *buffer, pos64 pos, size_t len, 
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

unsigned FileStream::WriteAt(const void *buffer, pos64 pos, size_t len, 
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

SeekableStream::pos64 FileStream::GetLength()
{
    DWORD hi;
    DWORD lo = GetFileSize(m_fd, &hi);
    if (lo == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
	return 0;
    return (((pos64)hi) << 32) + lo;
}

unsigned FileStream::SetLength(pos64 len)
{
    /* Although atomic pread() and pwrite are implemented (in NT-derived
     * Windows only), atomic ftruncate() is not -- except on Vista. :(
     */
    LONG hi = (LONG)(len >> 32);
    SetFilePointer(m_fd, (DWORD)len, &hi, FILE_BEGIN);
    SetEndOfFile(m_fd);
    return 0;
}

} // namespace win32

} // namespace util

#endif // WIN32
