#ifndef FILE_POSIX_H
#define FILE_POSIX_H 1

#include <string>
#include <vector>

namespace util {

struct Dirent;

/** Portability classes, Posix (Linux) versions (cf util::win32).
 */
namespace posix {

unsigned int Mkdir(const char *dirname);

bool DirExists(const char *dirname);

/** Create a relative symlink at "from", pointing at "to".
 *
 * Note that unlike the others, this one isn't "using'd" into
 * namespace util -- because it's inherently unportable, and readers
 * of code using it are thus made aware that it works on Posix only.
 */
unsigned MakeRelativeLink(const std::string& from, const std::string& to);

std::string Canonicalise(const std::string& path);

unsigned int ReadDirectory(const std::string& path, 
			   std::vector<util::Dirent> *entries);

} // namespace posix

} // namespace util

#endif
