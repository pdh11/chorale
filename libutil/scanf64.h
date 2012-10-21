#ifndef LIBUTIL_SCANF64_H
#define LIBUTIL_SCANF64_H 1

#include "config.h"
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdlib.h>

namespace util {

/** Like sscanf, but only deals with uint64_t integers.
 *
 * This horrible thing is needed because Mingw sscanf doesn't deal
 * with %llu, even though the compiler doesn't warn about it.
 */
int Scanf64(const char *input, const char *format, uint64_t *arg1,
	    uint64_t *arg2 = NULL, uint64_t *arg3 = NULL);

} // namespace util

#endif
