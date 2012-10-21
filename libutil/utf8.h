#ifndef LIBUTIL_UTF8_H
#define LIBUTIL_UTF8_H 1

#include <string>
#include <wchar.h>

namespace util {

#if WCHAR_MAX < 0x10000
typedef wchar_t utf16_t;
#else
typedef uint16_t utf16_t;
#endif

typedef std::basic_string<utf16_t> utf16string;

std::string UTF16ToUTF8(const utf16_t*);
inline std::string UTF16ToUTF8(const utf16string& s) { return UTF16ToUTF8(s.c_str()); }

utf16string UTF8ToUTF16(const char*);
inline utf16string UTF8ToUTF16(const std::string& s) { return UTF8ToUTF16(s.c_str()); }

} // namespace util

#endif
