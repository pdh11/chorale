#include "quote_escape.h"
#include <assert.h>
#include <string.h>

namespace util {

std::string QuoteEscape(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    for (unsigned int i=0; i<s.length(); ++i)
    {
	char c = s[i];
	if (c == '\"' || c == '\\')
	    result += '\\';
	result += c;
    }
    return result;
}

std::string QuoteUnEscape(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    const char *p = s.c_str();
    while (*p)
    {
	if (*p == '\\')
	    ++p;
	
	if (*p)
	{
	    result += *p;
	    ++p;
	}
    }
    return result;
}

} // namespace util

#ifdef TEST

int main()
{
    assert(util::QuoteEscape("foo") == "foo");
    assert(util::QuoteEscape("f\"oo") == "f\\\"oo");
    assert(util::QuoteEscape("f\\oo") == "f\\\\oo");
    assert(util::QuoteUnEscape("foo") == "foo");
    assert(util::QuoteUnEscape("f\\\"oo") == "f\"oo");
    assert(util::QuoteUnEscape("f\\\\oo") == "f\\oo");
    assert(util::QuoteUnEscape("f\\") == "f");
    return 0;
}

#endif
