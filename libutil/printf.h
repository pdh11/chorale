#ifndef LIBUTIL_PRINTF_H
#define LIBUTIL_PRINTF_H 1

#include <string>
#include <sstream>
#include <stdio.h>
#include "attributes.h"

namespace util {

std::string SPrintf(const char *fmt, ...)
    ATTRIBUTE_PRINTF(1,2);

struct Printf
{
    mutable std::stringstream ss;

    operator std::string () const { return ss.str(); }
};

template <typename T>
const Printf& operator<<(const Printf& p, T t)
{
    p.ss << t;
    return p;
}

} // namespace util

#endif
