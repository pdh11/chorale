#ifndef LIBUTIL_XMLESCAPE_H
#define LIBUTIL_XMLESCAPE_H

#include <string>

namespace util {

std::string XmlEscape(const std::string&);
std::string XmlUnEscape(const std::string&);

} // namespace util

#endif
