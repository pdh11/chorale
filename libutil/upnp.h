/* libutil/upnp.h */
#ifndef LIBUTIL_UPNP_H
#define LIBUTIL_UPNP_H 1

#include <stdio.h>

namespace util {

/** A user of libupnp (the Intel UPnP toolkit, not the Chorale library)
 *
 * Intel libupnp only allows for a single callback, so we must keep a list
 * of all users and call all their callbacks for them.
 */
class LibUPnPUser
{
public:
    LibUPnPUser();
    ~LibUPnPUser();

    size_t GetHandle();

    virtual int OnUPnPEvent(int event_type, void *event);
};

}; // namespace util

#endif
