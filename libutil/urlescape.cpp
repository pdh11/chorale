#include "urlescape.h"
#include "libutil/trace.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace util {

std::string URLEscape(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    for (unsigned int i=0; i<s.length(); ++i)
    {
	char c = s[i];
	if (c<33 || c > 126
	    || strchr("$&+,/:?@'<>\"#%", c) != NULL)
	{
	    char buf[4];
	    sprintf(buf, "%%%02x", c & 0xFF);
	    result += buf;
	}
	else
	    result += c;
    }
    return result;
}

std::string URLUnEscape(const std::string& s)
{
    std::string result;
    result.reserve(s.length());
    char buf[4];
    buf[2] = '\0';

    const char *p = s.c_str();
    while (*p)
    {
	if (*p == '%' && p[1] && p[2])
	{
	    buf[0] = p[1];
	    buf[1] = p[2];
	    result += (char)strtoul(buf, NULL, 16);
	    p += 3;
	}
	else
	{
	    result += *p;
	    ++p;
	}
    }
    return result;
}

} // namespace util

#ifdef TEST

#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

static const struct {
    const char *test;
    const char *expect;
} urltests[] = {
    { "foo", "foo" },
    { "808 State", "808%20State" },
    { "X&Y", "X%26Y" },
};

int main()
{
    for (unsigned int i=0; i<COUNTOF(urltests); ++i)
    {
	std::string result = util::URLEscape(urltests[i].test);
	if (result != urltests[i].expect)
	{
	    TRACE << "Escape(" << urltests[i].test
		  << ") = '" << result << "' should be '" << urltests[i].expect
		  << "')\n";
	    return 1;
	}
	std::string r2 = util::URLUnEscape(result);
	if (r2 != urltests[i].test)
	{
	    TRACE << "UnEscape(" << result
		  << ") = '" << r2 << "' should be '" << urltests[i].test
		  << "')\n";
	    return 1;
	}
    }
    return 0;
}

#endif
