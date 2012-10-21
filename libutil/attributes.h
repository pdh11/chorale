#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
/* Do we have at least GCC version X.Y? */
#define HAVE_GCC(X,Y) \
        ((__GNUC__ > (X)) || (__GNUC__ == (X) && __GNUC_MINOR__ >= (Y)))
#else
/* No, we don't */
#define HAVE_GCC(X,Y) 0
#endif

#if HAVE_GCC(3,0)
#define ATTRIBUTE(X) __attribute__(X)
#else
#define ATTRIBUTE(X)
#endif

#if HAVE_GCC(3,1)
#define ATTRIBUTE_DEPRECATED ATTRIBUTE((deprecated))
#else
#define ATTRIBUTE_DEPRECATED
#endif

#if HAVE_GCC(3,4)
#define ATTRIBUTE_WARNUNUSED ATTRIBUTE((warn_unused_result))
#else
#define ATTRIBUTE_WARNUNUSED
#endif

#endif
