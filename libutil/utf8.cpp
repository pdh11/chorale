#include "utf8.h"

namespace util {

#undef BOM
#undef ERROR

enum { BOM = 0xFEFF, ERROR = 0xFFFD };

std::string UTF16ToUTF8(const utf16_t *s)
{
    std::string result;
    result.reserve(64); // guess

    if (*s == BOM)
	++s;
    
    while (*s)
    {
	uint32_t ch = *s++;

	if (ch < 0x80)
	    result += (char)ch;
	else if (ch < 0x800)
	{
	    result += (char)(0xC0 + (ch>>6));
	    result += (char)(0x80 + (ch & 0x3F));
	}
	else
	{
	    if (ch >= 0xD800 && ch < 0xDC00) // Surrogate-pair
	    {
		if (*s >= 0xDC00 && *s < 0xE000)
		{
		    ch = 0x10000 + ((ch & 0x3FF) << 10);
		    ch += (*s & 0x3FF);
		    ++s;
		}
	    }
	    if (ch < 0x10000)
	    {
		result += (char)(0xE0 + (ch>>12));
		result += (char)(0x80 + ((ch>>6) & 0x3F));
		result += (char)(0x80 + (ch & 0x3F));
	    }
	    else
	    {
		result += (char)(0xF0 + (ch>>18));
		result += (char)(0x80 + ((ch>>12) & 0x3F));
		result += (char)(0x80 + ((ch>>6) & 0x3F));
		result += (char)(0x80 + (ch & 0x3F));
	    }
	}
    }

    return result;
}

static inline bool Cont(unsigned char c) { return ((c & 0xC0) == 0x80); }

utf16string UTF8ToUTF16(const char *s)
{
    utf16string result;
    result.reserve(64);
    bool last_error = false;

    const unsigned char *ptr = (const unsigned char*)s;

    while (*ptr)
    {
	unsigned char c = *ptr++;
	uint32_t wc;

	if (c < 0x80)
	{
	    wc = c;
	}
	else if ((c & 0xC0) == 0x80)
	{
	    wc = ERROR; // Continuation character (error)
	}
	else if ((c & 0xE0) == 0xC0)
	{
	    if (Cont(*ptr))
	    {
		wc = ((c & 0x1F) << 6)
		    + (*ptr & 0x3F);
		++ptr;

		if (wc < 0x80)
		    wc = ERROR;
	    }
	    else
		wc = ERROR;
	}
	else if ((c & 0xF0) == 0xE0)
	{
	    if (Cont(*ptr) && Cont(ptr[1]))
	    {
		wc = ((c & 0xF) << 12)
		    + ((*ptr & 0x3F)<<6)
		    + (ptr[1] & 0x3F);
		ptr += 2;

		if (wc < 0x800)
		    wc = ERROR;
	    }
	    else
		wc = ERROR;
	}
	else if ((c & 0xF8) == 0xF0)
	{
	    if (Cont(*ptr) && Cont(ptr[1]) && Cont(ptr[2]))
	    {
		wc = ((c & 0xF) << 18)
		    + ((*ptr & 0x3F)<<12)
		    + ((ptr[1] & 0x3F)<<6)
		    + (ptr[2] & 0x3F);
		ptr += 3;
		if (wc < 0x10000)
		    wc = ERROR;
	    }
	    else
		wc = ERROR;
	}
	else
	    wc = ERROR;

	if (wc != ERROR || last_error)
	{
	    if (wc >= 0x10000)
	    {
		uint32_t surrogate0 = 0xD800 + ((wc - 0x10000) >> 10);
		uint32_t surrogate1 = 0xDC00 + ((wc - 0x10000) & 0x3FF);
		result += (utf16_t)surrogate0;
		result += (utf16_t)surrogate1;
	    }
	    else
		result += (utf16_t)wc;
	}
	last_error = (wc == ERROR);
    }

    return result;
}

} // namespace util

#ifdef TEST

#include <assert.h>

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
    }

    return 0;
}

#endif
