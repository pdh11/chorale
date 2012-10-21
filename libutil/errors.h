#ifndef LIBUTIL_ERRORS_H
#define LIBUTIL_ERRORS_H 1

#include "config.h"
#include <errno.h>
#include <boost/cerrno.hpp>

enum {
    EDUMMY = 6000, // So that this still compiles if we have them all

#ifndef HAVE_EINVAL
    EINVAL,
#endif
#ifndef HAVE_EISCONN
    EISCONN,
#endif
#ifndef HAVE_EALREADY
    EALREADY,
#endif
#ifndef HAVE_EOPNOTSUPP
    EOPNOTSUPP,
#endif
#ifndef HAVE_EWOULDBLOCK
    EWOULDBLOCK,
#endif
#ifndef HAVE_EINPROGRESS
    EINPROGRESS,
#endif
    EDUMMY2
};

#endif
