#include "file_posix.h"
#include "file.h"
#include "trace.h"
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <assert.h>
#include <dirent.h>

#ifndef WIN32

namespace util {

namespace posix {

unsigned int Mkdir(const char *dirname)
{
    int rc = ::mkdir(dirname, 0775);
    if (rc < 0)
	return (unsigned)errno;
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
    char *cpath = canonicalize_file_name(path.c_str());
    if (!cpath)
	return path;

    std::string s(cpath);
    free(cpath);
    return s;
}

unsigned int ReadDirectory(const std::string& path, 
			   std::vector<Dirent> *entries)
{
    entries->clear();

    struct dirent **namelist;

    int rc = ::scandir(path.c_str(), &namelist, NULL, versionsort);
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

} // namespace posix

} // namespace util

#endif // !WIN32
