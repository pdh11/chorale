#ifndef LIBUTIL_UTF8_H
#define LIBUTIL_UTF8_H 1

#include <string>
#include <wchar.h>
#include <stdint.h>

namespace util {

#if WCHAR_MAX < 0x10000
typedef wchar_t utf16_t;
typedef uint32_t utf32_t;
#else
typedef uint16_t utf16_t;
typedef wchar_t utf32_t;
#endif

typedef std::basic_string<utf16_t> utf16string;
typedef std::basic_string<utf32_t> utf32string;

std::string UTF32ToUTF8(const utf32_t*);
inline std::string UTF32ToUTF8(const utf32string& s) { return UTF32ToUTF8(s.c_str()); }

utf32string UTF8ToUTF32(const char*);
inline utf32string UTF8ToUTF32(const std::string& s) { return UTF8ToUTF32(s.c_str()); }

std::string UTF16ToUTF8(const utf16_t*);
inline std::string UTF16ToUTF8(const utf16string& s) { return UTF16ToUTF8(s.c_str()); }

utf16string UTF8ToUTF16(const char*);
inline utf16string UTF8ToUTF16(const std::string& s) { return UTF8ToUTF16(s.c_str()); }

inline std::string WideToUTF8(const utf16_t* s) { return UTF16ToUTF8(s); }
inline std::string WideToUTF8(const utf16string& s) { return UTF16ToUTF8(s); }
inline std::string WideToUTF8(const utf32_t* s) { return UTF32ToUTF8(s); }
inline std::string WideToUTF8(const utf32string& s) { return UTF32ToUTF8(s); }

inline void UTF8ToWide(const char *i, utf16string *o) { *o = UTF8ToUTF16(i); }
inline void UTF8ToWide(const std::string& i, utf16string *o) { *o = UTF8ToUTF16(i); }
inline void UTF8ToWide(const char *i, utf32string *o) { *o = UTF8ToUTF32(i); }
inline void UTF8ToWide(const std::string& i, utf32string *o) { *o = UTF8ToUTF32(i); }

/** Convert text from the system local character encoding, to UTF-8.
 *
 * Strings in the system local character encoding are rare and
 * dangerous, and should be contained as closely as possible. Unlike
 * UTF-16 strings, they have the same C/C++ type as normal UTF-8
 * strings, so the compiler can't help you if they leak out into
 * portable code.
 *
 * Be very sure that you need this function before calling it. On
 * non-legacy Unix systems, the system local character encoding
 * already is UTF-8. About the only place this function is actually
 * needed, is when reading text files in legacy encodings, such as M3U
 * on Windows.
 */
std::string LocalEncodingToUTF8(const char*);

/** Convert text from UTF-8, to the system local character encoding.
 *
 * Strings in the system local character encoding are rare and
 * dangerous, and should be contained as closely as possible. Unlike
 * UTF-16 strings, they have the same C/C++ type as normal UTF-8
 * strings, so the compiler can't help you if they leak out into
 * portable code.
 *
 * Be very sure that you need this function before calling it. About
 * the only place this function is actually needed, is when writing
 * text files in legacy encodings, such as M3U on Windows.
 */
std::string UTF8ToLocalEncoding(const char*);

/** Extract one (wide) character from a UTF-8 string. Returns the Unicode
 * error character U+FFFD for encoding errors.
 *
 * Specialised for T=char, utf16_t, utf32_t.
 */
template<class T>
utf32_t GetChar(const T **const ppt);

template<>
utf32_t GetChar(const utf32_t **const ppt);

template<>
utf32_t GetChar(const uint16_t **const ppt);

template<>
utf32_t GetChar(const char **const ptr);

} // namespace util

#endif
