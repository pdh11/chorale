#include "config.h"
#include "tcp_wrappers_c.h"
#include <stdio.h>

#if HAVE_LIBWRAP

#include <tcpd.h>

int allow_severity;
int deny_severity;

int IsAllowed(unsigned int ipaddr_network, const char *daemon)
{
    struct request_info ri;
    char clientip[20];

    // NOLINTBEGIN we can't use snprintf_s, clang-tidy, 'cos it's not in glibc
    snprintf(clientip, sizeof(clientip), "%u.%u.%u.%u",
             ipaddr_network & 0xFF,
             (ipaddr_network >> 8) & 0xFF,
             (ipaddr_network >> 16) & 0xFF,
             ipaddr_network >> 24);
    // NOLINTEND

    request_init(&ri,
		 RQ_CLIENT_ADDR, clientip,
		 RQ_DAEMON, daemon,
		 0);
    return hosts_access(&ri);
}

#else

int IsAllowed(unsigned int ipaddr_network, const char *daemon)
{
    (void)(ipaddr_network);
    (void)(daemon);
    return 1;
}

#endif
