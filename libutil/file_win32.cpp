#include "file_win32.h"
#include "file.h"
#include "utf8.h"
#include "trace.h"

#ifdef WIN32

#include <io.h>
#include <shlwapi.h>
#include <sys/stat.h>

namespace util {

namespace win32 {

unsigned int Mkdir(const char *dirname)
{
    std::wstring utf16 = UTF8ToUTF16(dirname);
    int rc = ::_wmkdir(utf16.c_str());
    if (rc < 0)
	return (unsigned)errno;
    return 0;
}

bool DirExists(const char *dirname)
{
    std::wstring utf16 = UTF8ToUTF16(dirname);
    struct _stat st;
    int rc = ::_wstat(utf16.c_str(), &st);
    return (rc == 0) && S_ISDIR(st.st_mode);
}

std::string Canonicalise(const std::string& path)
{
    std::wstring utf16 = UTF8ToUTF16(path);

    wchar_t canon[MAX_PATH];

    if (::PathCanonicalizeW(canon, utf16.c_str()))
	return UTF16ToUTF8(canon);
    return path;
}

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

} // namespace win32

} //  namespace util

#endif // WIN32
