#include "printf.h"
#include "config.h"
#include "trace.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

std::string util::SPrintf(const char *fmt, ...)
{
    va_list va;

#if HAVE_VASPRINTF
    char *ptr = NULL;

    va_start(va, fmt);

    int rc = vasprintf(&ptr, fmt, va);

    va_end(va);

    if (rc<0)
	return std::string();

    std::string s(ptr);
    free(ptr);
#elif HAVE___MINGW_VSNPRINTF
    
    va_start(va, fmt);
    int rc = __mingw_vsnprintf(NULL, 0, fmt, va);
    va_end(va);

    if (rc < 0)
	return std::string();

    char *ptr = (char*)malloc(rc+1);
    va_start(va, fmt);
    __mingw_vsnprintf(ptr, rc+1, fmt, va);
    va_end(va);

    std::string s(ptr);
    free(ptr);
#else
#error "Can't implement SPrintf on this platform"
#endif

    return s;
}

#ifdef TEST

std::string foo()
{
    return util::Printf() << 37 << "\n";
}

int main()
{
    std::string s = foo();

    TRACE << "s='" << s << "'\n";

    assert(s == "37\n");

    std::string s3 = "Long long long long long long";

    std::string s2 = util::Printf() << 2 << s3 << 3;
    
    printf("%s\n", s2.c_str());

    assert(s2 == "2"+s3+"3");

    return 0;
}

#endif
