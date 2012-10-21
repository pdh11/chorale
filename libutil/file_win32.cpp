#include "file_win32.h"
#include "config.h"
#include "utf8.h"
#include "file.h"
#include "trace.h"
#include <algorithm>
#if HAVE_IO_H
#include <io.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_WINDOWS_H
#include <windows.h>
#endif

namespace util {

namespace win32 {

std::string GetLeafName(const char *filename)
{
    const char *rslash = strrchr(filename, '/');
    const char *backslash = strrchr(filename, '\\');
    if (backslash && (!rslash || backslash > rslash))
	rslash = backslash;
    if (rslash)
	return std::string(rslash+1);
    return filename;
}

std::string GetDirName(const char *filename)
{
    const char *rslash = strrchr(filename, '/');
    const char *backslash = strrchr(filename, '\\');
    if (backslash && (!rslash || backslash > rslash))
	rslash = backslash;
    if (rslash)
	return std::string(filename, rslash);
    return std::string();
}

std::string GetExtension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot)
	return "";
    const char *slash = strrchr(filename, '/');
    if (slash && slash > dot)
	return "";
    const char *backslash = strrchr(filename, '\\');
    if (backslash && backslash > dot)
	return "";
    return std::string(dot+1);
}

std::string StripExtension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot)
	return filename;
    const char *slash = strrchr(filename, '/');
    if (slash && slash > dot)
	return filename;
    const char *backslash = strrchr(filename, '\\');
    if (backslash && backslash > dot)
	return filename;
    return std::string(filename, dot);
}

#if HAVE__WMKDIR
unsigned int Mkdir(const char *dirname)
{
    std::wstring utf16 = UTF8ToUTF16(dirname);
    int rc = ::_wmkdir(utf16.c_str());
    if (rc < 0)
	return (unsigned)errno;
    return 0;
}
#endif

#if HAVE__WSTAT
bool DirExists(const char *dirname)
{
    std::wstring utf16 = UTF8ToUTF16(dirname);
    struct _stat st;
    int rc = ::_wstat(utf16.c_str(), &st);
    return (rc == 0) && S_ISDIR(st.st_mode);
}
#endif

#if HAVE_GETFULLPATHNAMEW
std::string Canonicalise(const std::string& path)
{
    std::wstring utf16 = UTF8ToUTF16(path);

    wchar_t canon[MAX_PATH];

    /** Note that PathCanonicalize does NOT do what we want here -- it's a
     * purely textual operation that eliminates /./ and /../ only.
     */
    DWORD rc = ::GetFullPathNameW(utf16.c_str(), MAX_PATH, canon, NULL);
    if (!rc)
	return path;

    rc = ::GetLongPathNameW(canon, canon, MAX_PATH);
    if (!rc)
	return path;

    std::string utf8 = UTF16ToUTF8(canon);
    std::replace(utf8.begin(), utf8.end(), '\\', '/');
    return utf8;
}
#endif

#if HAVE_FINDFIRSTFILEW
static uint32_t FileTimeToTimeT(const FILETIME& ft)
{
    uint64_t ftt = ((uint64_t)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
    return (uint32_t)(( ftt - 0x19DB1DED53E8000ull ) / 10000000ull);
}

unsigned int ReadDirectory(const std::string& path, 
			   std::vector<Dirent> *entries)
{
    entries->clear();

//    TRACE << "Reading '" << path << "'\n";

    std::wstring ux16(UTF8ToUTF16(path));
    std::wstring utf16find(ux16 + L"/*");

    WIN32_FIND_DATAW ffd;

    HANDLE handle = ::FindFirstFileW(utf16find.c_str(), &ffd);

    if (handle == INVALID_HANDLE_VALUE)
    {
	unsigned int rc = GetLastError();
	TRACE << "Can't open dir: " << rc << "\n";
	return rc;
    }

    do {
	if (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
	    continue;

	Dirent d;
	d.name = UTF16ToUTF8(ffd.cFileName);

	if (d.name == ".." || d.name == ".")
	    continue;

	TRACE << "  /" << d.name << "\n";

	d.st.st_size = ffd.nFileSizeLow;
	d.st.st_mtime = FileTimeToTimeT(ffd.ftLastWriteTime);
	if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    d.st.st_mode = 0755 | S_IFDIR; 
	else
	    d.st.st_mode = 0644 | S_IFREG;
	
	entries->push_back(d);
    } while (::FindNextFileW(handle, &ffd) != 0);
    
    ::FindClose(handle);

    return 0;
}
#endif

} // namespace win32

} //  namespace util

