#ifndef LIBUTIL_FILE_H
#define LIBUTIL_FILE_H

#include <string>
#include <sys/stat.h>

#include "file_posix.h"
#include "file_win32.h"


/** Utility classes and routines which didn't fit anywhere else.
 */
namespace util {

namespace posix { }
namespace win32 { }

#ifdef WIN32
namespace fileapi = ::util::win32;
#else
namespace fileapi = ::util::posix;
#endif

using fileapi::Mkdir;
using fileapi::DirExists;
using fileapi::GetDirName;
using fileapi::GetLeafName;
using fileapi::Canonicalise;
using fileapi::GetExtension;
using fileapi::ReadDirectory;
using fileapi::StripExtension;

/** Create all the parent directories needed so that file "leafname" can be
 * created.
 *
 * Like "mkdir -p `dirname $leafname`".
 */
unsigned int MkdirParents(const char *leafname);

void RenameWithMkdir(const char *oldname, const char *newname);

/** Replaces "awkward" characters (such as "/") with filesystem-safe
 * alternatives.
 */
std::string ProtectLeafname(const std::string& s);

std::string MakeAbsolutePath(const std::string& start_point,
			     const std::string& relative_path);

std::string MakeRelativePath(const std::string& abspath_from,
			     const std::string& abspath_to);

/** Creates a "file:///" URL
 */
std::string PathToURL(const std::string& path);

/** Parses a "file:///" URL
 */
std::string URLToPath(const std::string& url);

struct Dirent
{
    std::string name;
    struct stat st;
};

} // namespace util

#endif
