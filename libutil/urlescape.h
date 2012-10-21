#ifndef LIBUTIL_URLESCAPE_H
#define LIBUTIL_URLESCAPE_H 1

#include <string>

namespace util {

std::string URLEscape(const std::string&);
std::string URLUnEscape(const std::string&);

} // namespace util

#endif
