#ifndef CHORALECD_FEATURES_H
#define CHORALECD_FEATURES_H

#include "config.h"

#if defined(HAVE_GSTREAMER)
#define HAVE_LIBOUTPUT 1
#endif
#if defined(HAVE_LAME) && defined(HAVE_LIBFLAC) && defined(HAVE_LIBCDDB) && (defined(HAVE_LIBCDIOP) || defined(HAVE_PARANOIA))
#define HAVE_CD 1
#endif

#endif
