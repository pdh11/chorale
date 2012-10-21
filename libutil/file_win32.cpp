#include "file_win32.h"
#include "file.h"
#include "utf8.h"

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

unsigned int ReadDirectory(const std::string& path, 
			   std::vector<Dirent> *entries)
{
    entries->clear();

    std::wstring utf16 = UTF8ToUTF16(path);
    std::wstring utf16find = utf16;
    utf16find += (utf16_t)'/';
    utf16find += (utf16_t)'*';

    struct _wfinddatai64_t ffd;

    intptr_t handle = ::_wfindfirsti64(utf16find.c_str(), &ffd);

    if (handle == (intptr_t)-1)
	return (unsigned)errno;

    do {
	Dirent d;
	d.name = UTF16ToUTF8(ffd.name);
	
	utf16string childpath = utf16;
	childpath += (utf16_t)'/';
	childpath += ffd.name;
	::_wstat(childpath.c_str(), (struct _stat*)&d.st);
	entries->push_back(d);

    } while (::_wfindnexti64(handle, &ffd) == 0);

    _findclose(handle);

    return 0;
}

} // namespace win32

} //  namespace util

#endif // WIN32
