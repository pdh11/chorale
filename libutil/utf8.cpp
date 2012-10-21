#include "utf8.h"
#include <string.h>
#include <stdlib.h>

namespace util {

#undef BOM
#undef ERROR

static const utf32_t ERROR = 0xFFFD;
static const utf32_t BOM   = 0xFEFF;

static inline bool Cont(unsigned char c) { return ((c & 0xC0) == 0x80); }


        /* GetChar */


template<>
utf32_t GetChar(const utf32_t **const ppt)
{
    return *(*ppt)++;
}

template<>
utf32_t GetChar(const utf16_t **const ppt)
{
    utf32_t ch = *(*ppt)++;
    if (ch >= 0xD800 && ch < 0xDC00) // Surrogate-pair
    {
	utf32_t ch2 = **ppt;
	if (ch2 >= 0xDC00 && ch2 < 0xE000)
	{
	    ch = 0x10000 + ((ch & 0x3FF) << 10);
	    ch += (ch2 & 0x3FF);
	    ++(*ppt);
	}
	else
	    return ERROR;
    }
    return ch;
}

template<>
utf32_t GetChar(const char **const ptr)
{
    unsigned char c = *(*ptr)++;
    utf32_t wc;

    if (c < 0x80)
    {
	return c;
    }
    if ((c & 0xC0) == 0x80)
    {
	return ERROR; // Continuation character (error)
    }
    if ((c & 0xE0) == 0xC0)
    {
	if (Cont(**ptr))
	{
	    wc = ((c & 0x1F) << 6)
		+ (**ptr & 0x3F);
	    ++(*ptr);

	    return (wc < 0x80) ? ERROR : wc;
	}
	return ERROR;
    }
    if ((c & 0xF0) == 0xE0)
    {
	if (Cont(**ptr) && Cont((*ptr)[1]))
	{
	    wc = ((c & 0xF) << 12)
		+ ((**ptr & 0x3F)<<6)
		+ ((*ptr)[1] & 0x3F);
	    (*ptr) += 2;

	    return (wc < 0x800) ? ERROR : wc;
	}
	return ERROR;
    }
    if ((c & 0xF8) == 0xF0)
    {
	if (Cont(**ptr) && Cont((*ptr)[1]) && Cont((*ptr)[2]))
	{
	    wc = ((c & 0xF) << 18)
		+ ((**ptr & 0x3F)<<12)
		+ (((*ptr)[1] & 0x3F)<<6)
		+ ((*ptr)[2] & 0x3F);
	    (*ptr) += 3;

	    return (wc < 0x10000) ? ERROR : wc;
	}
    }
    return ERROR;
}


        /* PutChar */


template<class T>
static inline void PutChar(utf32_t ch, std::basic_string<T> *s);

template<>
void PutChar(utf32_t ch, utf32string *s)
{
    (*s) += ch;
}

template<>
void PutChar(utf32_t ch, utf16string *s)
{
    if (ch >= 0x10000)
    {
	utf32_t surrogate0 = 0xD800 + ((ch - 0x10000) >> 10);
	utf32_t surrogate1 = 0xDC00 + ((ch - 0x10000) & 0x3FF);
	(*s) += (utf16_t)surrogate0;
	(*s) += (utf16_t)surrogate1;
    }
    else
	(*s) += (utf16_t)ch;
}

template<>
void PutChar(utf32_t ch, std::string *s)
{
    if (ch < 0x80)
	(*s) += (char)ch;
    else if (ch < 0x800)
    {
	(*s) += (char)(0xC0 + (ch>>6));
	(*s) += (char)(0x80 + (ch & 0x3F));
    }
    else if (ch < 0x10000)
    {
	(*s) += (char)(0xE0 + (ch>>12));
	(*s) += (char)(0x80 + ((ch>>6) & 0x3F));
	(*s) += (char)(0x80 + (ch & 0x3F));
    }
    else
    {
	(*s) += (char)(0xF0 + (ch>>18));
	(*s) += (char)(0x80 + ((ch>>12) & 0x3F));
	(*s) += (char)(0x80 + ((ch>>6) & 0x3F));
	(*s) += (char)(0x80 + (ch & 0x3F));
    }
}


        /* Actual conversions */


template <class X, class Y>
std::basic_string<X> Convert(const Y *s)
{
    std::basic_string<X> result;
    result.reserve(64); // guess
    bool last_error = false;
    
    for (;;)
    {
	utf32_t ch = GetChar(&s);
	if (!ch)
	    break;

	if (ch == BOM)
	    continue;

	if (ch != ERROR || !last_error)
	    PutChar(ch, &result);

	last_error = (ch == ERROR);
    }

    return result;
}

std::string UTF32ToUTF8(const utf32_t *s)
{
    return Convert<char>(s);
}

std::string UTF16ToUTF8(const utf16_t *s)
{
    return Convert<char>(s);
}

utf16string UTF8ToUTF16(const char *s)
{
    return Convert<utf16_t>(s);
}

utf32string UTF8ToUTF32(const char *s)
{
    return Convert<utf32_t>(s);
}

std::string LocalEncodingToUTF8(const char *local_string)
{
    size_t len_chars = mbstowcs(NULL, local_string, 0);

    if (len_chars == (size_t)-1)
	return local_string;

    std::wstring ws;
    ws.resize(len_chars);

    mbstowcs(&ws[0], local_string, len_chars+1);

    return WideToUTF8(ws);
}

std::string UTF8ToLocalEncoding(const char *utf8_string)
{
    std::wstring ws;
    UTF8ToWide(utf8_string, &ws);

    size_t len_bytes = wcstombs(NULL, ws.c_str(), 0);

    if (len_bytes == (size_t)-1)
	return utf8_string;

    std::string s;
    s.resize(len_bytes);

    wcstombs(&s[0], ws.c_str(), len_bytes+1);

    return s;
}

} // namespace util

#ifdef TEST

# include <assert.h>
# include <algorithm>

const util::utf16_t test1[] = { 'f', 'o', 'o', 0 };
const util::utf16_t test2[] = { 'b', 'e', 'y', 'o', 'n', 'c', 0xE9, 0 };
const util::utf16_t test3[] = { 0xC1, 'g', 0xE6, 't', 'i', 's', 0 };
const util::utf16_t test4[] = { 0x6CB3, 0x8C5A, 0 };

const struct {
    const char *utf8;
    const util::utf16_t *utf16;
} tests[] = {

    { "foo", test1 },
    { "beyonc\xC3\xA9", test2 },
    { "\xc3\x81g\xc3\xa6tis", test3 },
    { "\xE6\xB2\xB3\xE8\xB1\x9A", test4 }, // O'Reilly CJKV p194

};

enum { NTESTS = sizeof(tests)/sizeof(tests[0]) };

int main()
{
    for (unsigned int i=0; i<NTESTS; ++i)
    {
	std::string utf8 = tests[i].utf8;
	util::utf16string utf16 = tests[i].utf16;
	assert(util::UTF8ToUTF16(utf8) == utf16);
	assert(util::UTF16ToUTF8(utf16) == utf8);

	util::utf32string utf32;
	std::copy(utf16.begin(), utf16.end(), 
		  std::back_insert_iterator<util::utf32string>(utf32));

	assert(util::UTF8ToUTF32(utf8) == utf32);
	assert(util::UTF32ToUTF8(utf32) == utf8);
    }

    return 0;
}

#endif
