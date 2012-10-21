#ifndef LIBUTIL_TCP_WRAPPERS_H
#define LIBUTIL_TCP_WRAPPERS_H 1

#include "ip.h"
#include "locking.h"
#include <map>
#include <time.h>

namespace util {

class TcpWrappers: public PerClassLocking<TcpWrappers>
{
    const char *m_daemon;
    time_t m_allow_mtime;
    time_t m_deny_mtime;
    time_t m_check_time;
    typedef std::map<uint32_t, bool> map_t;
    map_t m_map;

    enum {
	CHECK_TIMESTAMPS = 30, ///< Time between checking file mtimes (sec)
	CACHE_TIMEOUT = 600    ///< Throw away entire cache every n seconds
    };

public:
    explicit TcpWrappers(const char *daemon);
    ~TcpWrappers();

    bool Allowed(IPAddress client);
};

} // namespace util

#endif
