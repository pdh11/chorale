#ifndef FILE_WIN32_H
#define FILE_WIN32_H 1

#include <string>
#include <vector>

namespace util {

struct Dirent;

/** Portability classes, Win32 versions (cf util::posix).
 */
namespace win32 {

unsigned int Mkdir(const char *dirname);

bool DirExists(const char *dirname);

std::string Canonicalise(const std::string& path);

unsigned int ReadDirectory(const std::string& path, 
			   std::vector<util::Dirent> *entries);


} // namespace win32

} // namespace util

#endif
