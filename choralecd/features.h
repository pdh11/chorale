#ifndef CHORALECD_FEATURES_H
#define CHORALECD_FEATURES_H

#include "config.h"

#if defined(HAVE_CURL)
#define HAVE_LIBDBRECEIVER 1
#endif
#if defined(HAVE_UPNP)
#define HAVE_LIBDBUPNP 1
#endif
#if defined(HAVE_UPNP)
#define HAVE_LIBOUTPUT_UPNP 1
#endif
#if defined(HAVE_GSTREAMER)
#define HAVE_LIBOUTPUT 1
#endif
#if defined(HAVE_LAME) && defined(HAVE_LIBFLAC) && defined(HAVE_LIBCDDB) && defined(HAVE_LIBCDIOP)
#define HAVE_CD 1
#endif

#endif
