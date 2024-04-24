#include "file_win32.h"
#include "config.h"
#include "utf8.h"
#include "file.h"
#include "trace.h"
#include <algorithm>
#include <string.h>
#if 0
#if HAVE_IO_H
#include <io.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_WINDOWS_H
#include <windows.h>
#endif
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

#if 0
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

    utf16 = canon;

    if (utf16.length() >= 6)
    {
	/** Get rid of \\?\ and \\.\ prefixes on drive-letter paths */
	if (!wcsncmp(utf16.c_str(), L"\\\\?\\", 4) && utf16[5] == L':')
	    utf16.erase(0,4);
	else if (!wcsncmp(utf16.c_str(), L"\\\\.\\", 4) && utf16[5] == L':')
	    utf16.erase(0,4);
    }

    if (utf16.length() >= 10)
    {
	/** Get rid of \\?\UNC on drive-letter and UNC paths */
	if (!wcsncmp(utf16.c_str(), L"\\\\?\\UNC\\", 8))
	{
	    if (utf16[9] == L':' && utf16[10] == L'\\')
		utf16.erase(0,8);
	    else
	    {
		utf16.erase(0,7);
		utf16 = L"\\" + utf16;
	    }
	}
    }

    /** Anything other than UNC and drive-letter is something we don't
     * understand
     */
    if (utf16[0] == L'\\' && utf16[1] == L'\\')
    {
	if (utf16[2] == '?' || utf16[2] == '.')
	    return path; // Not understood

	/** OK -- UNC */
    }
    else if (((utf16[0] >= 'A' && utf16[0] <= 'Z')
	      || (utf16[0] >= 'a' && utf16[0] <= 'z'))
	     && utf16[1] == ':')
    {
	/** OK -- drive letter -- unwind subst'ing */
	for (;;)
	{
	    wchar_t drive[3];
	    drive[0] = (wchar_t)toupper(utf16[0]);
	    drive[1] = L':';
	    drive[2] = L'\0';
	    canon[0] = L'\0';
	    rc = ::QueryDosDeviceW(drive, canon, MAX_PATH);
	    if (!rc)
		break;
	    if (!wcsncmp(canon, L"\\??\\", 4))
	    {
		utf16 = std::wstring(canon+4) + std::wstring(utf16, 2);
	    }
	    else // Not subst'd
		break;
	}

	wchar_t drive[4];
	drive[0] = (wchar_t)toupper(utf16[0]);
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = '\0';

	rc = ::GetDriveTypeW(drive);

	if (rc == DRIVE_REMOTE)
	{
	    DWORD bufsize = MAX_PATH;

	    /* QueryDosDevice and WNetGetConnection FORBID the
	     * trailing slash; GetDriveType REQUIRES it.
	     */
	    drive[2] = '\0';

	    rc = ::WNetGetConnectionW(drive, canon, &bufsize);
	    if (rc == NO_ERROR)
		utf16 = std::wstring(canon) + std::wstring(utf16, 2);
	}
    }
    else
    {
	// Not understood
	return path;
    }

    /** Canonicalise case and 8.3-ness */
    rc = ::GetLongPathNameW(utf16.c_str(), canon, MAX_PATH);
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
#endif

} // namespace win32

} //  namespace util

#ifdef WIN32

/* Wrappers for system calls that take narrow filenames */

extern "C" FILE* __cdecl __wrap_fopen(const char *path, const char *mode)
{
    std::wstring wpath, wmode;
    util::UTF8ToWide(path, &wpath);
    util::UTF8ToWide(mode, &wmode);
    return _wfopen(wpath.c_str(), wmode.c_str());
}

extern "C" int __cdecl __wrap_rename(const char *oldname, const char *newname)
{
    std::wstring woldname, wnewname;
    util::UTF8ToWide(oldname, &woldname);
    util::UTF8ToWide(newname, &wnewname);
    return _wrename(woldname.c_str(), wnewname.c_str());
}

extern "C" int __cdecl __wrap_unlink(const char *delendum)
{
    std::wstring wdelendum;
    util::UTF8ToWide(delendum, &wdelendum);
    return _wunlink(wdelendum.c_str());
}

#endif
