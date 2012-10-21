#include <string.h>

#ifdef WIN32

/** GCC 4.3.0 miscompiles the wmemcpy in mingw-runtime-3.14, causing all hell
 * to break loose. Use this, much simpler, wmemcpy instead.
 */
extern "C" wchar_t *__wrap_wmemcpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    return (wchar_t*)memcpy(dest, src, n*2);
}

/** GCC 4.3.0 miscompiles the wmemcpy in mingw-runtime-3.14, causing
 * all hell to break loose. Just in case wmemmove is similarly
 * affected, use this, much simpler, one instead.
 */
extern "C" wchar_t *__wrap_wmemmove(wchar_t *dest, const wchar_t *src, size_t n)
{
    return (wchar_t*)memmove(dest, src, n*2);
}

#endif
