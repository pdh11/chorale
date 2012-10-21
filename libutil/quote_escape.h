#ifndef LIBUTIL_QUOTE_ESCAPE_H
#define LIBUTIL_QUOTE_ESCAPE_H 1

#include <string>

namespace util {

std::string QuoteEscape(const std::string&);
std::string QuoteUnEscape(const std::string&);

} // namespace util

#endif
