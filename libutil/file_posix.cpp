#include "file_posix.h"
#include "file.h"
#include "config.h"
#include "trace.h"
#include "compare.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>

namespace util {

namespace posix {

std::string GetLeafName(const char *filename)
{
    const char *rslash = strrchr(filename, '/');
    if (rslash)
	return std::string(rslash+1);
    return filename;
}

std::string GetDirName(const char *filename)
{
    const char *rslash = strrchr(filename, '/');
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
    return std::string(filename, dot);
}


#ifndef WIN32
unsigned int Mkdir(const char *dirname)
{
    int rc = ::mkdir(dirname, 0775);
    if (rc < 0)
    {
	TRACE << "mkdir(" << dirname << " failed: " << errno << "\n";
	return (unsigned)errno;
    }
    return 0;
}

bool DirExists(const char *dirname)
{
    struct stat st;
    int rc = ::stat(dirname, &st);
    return (rc == 0) && S_ISDIR(st.st_mode);
}

unsigned MakeRelativeLink(const std::string& from, const std::string& to)
{
    if (from.empty() || to.empty() || from[0] != '/' || to[0] != '/')
	return EINVAL;

    std::string thelink = MakeRelativePath(from, to);

//    TRACE << "from=" << from << " to=" << to << " link=" << thelink << "\n";

    int rc = symlink(thelink.c_str(), from.c_str());
    if (rc < 0)
	return (unsigned)errno;
    return 0;
}

std::string Canonicalise(const std::string& path)
{
//    TRACE << "Canonicalising " << path << "\n";
#if HAVE_CANONICALIZE_FILE_NAME
    char *cpath = canonicalize_file_name(path.c_str());
#elif HAVE_REALPATH
    char buffer[PATH_MAX];
    char *cpath = realpath(path.c_str(), buffer);
#else
    char *cpath = NULL;
#endif
    if (!cpath)
	return path;

//    TRACE << "Canonicalise(" << path << ")=(" << cpath << ")\n";

    std::string s(cpath);
#if HAVE_CANONICALIZE_FILE_NAME
    free(cpath);
#endif
    return s;
}

static int ScandirCompare(SCANDIR_COMPARATOR_ARG_T v1,
			  SCANDIR_COMPARATOR_ARG_T v2)
{
    const struct dirent *d1 = *(const struct dirent**)v1;
    const struct dirent *d2 = *(const struct dirent**)v2;
    return util::Compare(d1->d_name, d2->d_name, true);
}

unsigned int ReadDirectory(const std::string& path, 
			   std::vector<Dirent> *entries)
{
    entries->clear();

    struct dirent **namelist;

    int rc = ::scandir(path.c_str(), &namelist, NULL, &ScandirCompare);
    if (rc < 0)
	return (unsigned) errno;
    if (rc == 0)
	return 0;

    entries->reserve(rc);

    for (int i=0; i<rc; ++i)
    {
	struct dirent *de = namelist[i];
	Dirent d;
	d.name = de->d_name;
	free(de);

	std::string childpath = path + "/" + d.name;
	::lstat(childpath.c_str(), &d.st);
	
	entries->push_back(d);
    }

    free(namelist);

    return 0;
}
#endif

} // namespace posix

} // namespace util
