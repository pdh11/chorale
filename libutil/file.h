#ifndef LIBUTIL_FILE_H
#define LIBUTIL_FILE_H

#include <string>

/** Utility classes and routines which didn't fit anywhere else.
 */
namespace util {

void MkdirParents(const char *leafname);

void RenameWithMkdir(const char *oldname, const char *newname);

std::string ProtectLeafname(const std::string& s);

std::string GetDirName(const char *filename);
std::string GetLeafName(const char *filename);

/** Returns file extension (without the ".") */
std::string GetExtension(const char *filename);

std::string StripExtension(const char *filename);

std::string MakeAbsolutePath(const std::string& start_point,
			     const std::string& relative_path);

std::string MakeRelativePath(const std::string& abspath_from,
			     const std::string& abspath_to);

unsigned MakeRelativeLink(const std::string& abspath_from,
			  const std::string& abspath_to);

std::string Canonicalise(const std::string& path);

/** Creates a "file:///" URL
 */
std::string PathToURL(const std::string& path);

/** Parses a "file:///" URL
 */
std::string URLToPath(const std::string& url);

}; // namespace util

#endif
