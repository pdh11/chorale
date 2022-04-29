#include "scanf64.h"
#include "trace.h"
#include <stdlib.h>
#include <ctype.h>

namespace util {

int Scanf64(const char *input, const char *format, uint64_t *arg1,
	    uint64_t *arg2, uint64_t *arg3)
{
    int matches = 0;
    
    const char *ptr = format;

    while (*ptr && *input)
    {
	//TRACE << "'" << ptr << "' vs '" << input << "'\n";

	if (isspace(*ptr))
	{
	    ++ptr;
	    // Whitespace matches an arbitrary amount of whitespace
	    while (*input && isspace(*input))
		++input;
	}
	else if (*ptr == '%')
	{
	    if (ptr[1] != 'l' || ptr[2] != 'l' || ptr[3] != 'u')
	    {
		TRACE << "Unhandled format: " << ptr << "\n";
		return 0;
	    }
	    
	    ptr += 4;

	    if (matches == 3)
	    {
		TRACE << "Too many %llu's in " << format << "\n";
		return 0;
	    }

	    char c = *input;

	    if (c < '0' || c > '9')
		return 0; // Doesn't match

	    uint64_t result = 0;
	    
	    do {
		result = (result*10) + (c - '0');
		++input;
		c = *input;
	    } while (c >= '0' && c <= '9');

	    uint64_t *destination = NULL;
	    if (matches == 0)
		destination = arg1;
	    else if (matches == 1)
		destination = arg2;
	    else if (matches == 2)
		destination = arg3;

	    if (!destination)
	    {
		TRACE << "Not enough arguments for %llu's in " << format << "\n";
		return 0;
	    }

	    *destination = result;
	    ++matches;
	}
	else
	{
	    // Not whitespace or '%': must be literal match
	    if (*input != *ptr)
		return 0;

	    ++input;
	    ++ptr;
	}
    }

    // Note that we return matches here even if there's unmatched stuff in
    // the format. That's what real scanf does.
    return matches;
}

}

#ifdef TEST

# include <assert.h>

static void Test1(const char *input, const char *format,
                  int res_expect, uint64_t arg1_expect)
{
    uint64_t arg1 = ~arg1_expect;
    int res = util::Scanf64(input, format, &arg1);
    assert(res  == res_expect);
    if (res == 1)
	assert(arg1 == arg1_expect);
}

static void Test2(const char *input, const char *format,
                  int res_expect, uint64_t arg1_expect, uint64_t arg2_expect)
{
    uint64_t arg1 = ~arg1_expect;
    uint64_t arg2 = ~arg2_expect;
    int res = util::Scanf64(input, format, &arg1, &arg2);
    assert(res  == res_expect);
    if (res >= 1)
	assert(arg1 == arg1_expect);
    if (res >= 2)
	assert(arg2 == arg2_expect);
}

static void Test3(const char *input, const char *format,
                  int res_expect, uint64_t arg1_expect, uint64_t arg2_expect,
                  uint64_t arg3_expect)
{
    uint64_t arg1 = ~arg1_expect;
    uint64_t arg2 = ~arg2_expect;
    uint64_t arg3 = ~arg3_expect;
    int res = util::Scanf64(input, format, &arg1, &arg2, &arg3);
    assert(res  == res_expect);
    if (res >= 1)
	assert(arg1 == arg1_expect);
    if (res >= 2)
	assert(arg2 == arg2_expect);
    if (res >= 3)
	assert(arg3 == arg3_expect);
}

int main()
{
    Test1("0", "%llu", 1, 0);
    Test1("x0", "x%llu", 1, 0);
    Test1("0xF", "%llu", 1, 0);
    Test1("x83572239y", "x%lluy", 1, 83572239);
    Test1("x", "%llu", 0, 0);

    Test2("4-",         "bytes=%llu-%llu", 0, 0, 0);
    Test2("4-9",        "bytes=%llu-%llu", 0, 0, 0);
    Test2("bytes=4-",   "bytes=%llu-%llu", 1, 4, 0);
    Test2("bytes=4-9",  "bytes=%llu-%llu", 2, 4, 9);
    Test2("bytes  4-9", "bytes %llu-%llu", 2, 4, 9);
    Test2("bytes  4-9 ", "bytes %llu-%llu", 2, 4, 9);
    Test2("bytes  4-9 ", "bytes %llu-%llu ", 2, 4, 9);

    Test3("bytes 0-10/21", "bytes %llu-%llu/%llu", 3, 0, 10, 21);

    return 0;
}

#endif
