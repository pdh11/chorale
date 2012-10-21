#ifndef LIBUTIL_TCP_WRAPPERS_C_H
#define LIBUTIL_TCP_WRAPPERS_C_H

/** The hosts_access(3) API is so ropey that you can't even do it from C++.
 */
int IsAllowed(unsigned int ipaddr_networkbyteorder, const char *daemon);

#endif
