#include "config.h"
#include "tcp_wrappers.h"
extern "C" {
#include "tcp_wrappers_c.h"
}
#include "trace.h"
#include <sys/stat.h>

namespace util {

TcpWrappers::TcpWrappers(const char *daemon)
    : m_daemon(daemon),
      m_allow_mtime(0),
      m_deny_mtime(0),
      m_check_time(0)
{
}

bool TcpWrappers::Allowed(IPAddress client)
{
#if HAVE_LIBWRAP
    Lock lock(this);

    time_t now = ::time(NULL);

    if ((now - m_check_time) > CACHE_TIMEOUT)
	m_map.clear();

    if ((now - m_check_time) > CHECK_TIMESTAMPS)
    {
	struct stat st;
	if (::stat("/etc/hosts.allow", &st) == 0)
	{
	    if (st.st_mtime != m_allow_mtime)
	    {
		m_allow_mtime = st.st_mtime;
		m_map.clear();
	    }
	}
	if (::stat("/etc/hosts.deny", &st) == 0)
	{
	    if (st.st_mtime != m_deny_mtime)
	    {
		m_deny_mtime = st.st_mtime;
		m_map.clear();
	    }
	}
    }

    map_t::const_iterator i = m_map.find(client.addr);
    if (i != m_map.end())
	return i->second;

    bool allowed = ::IsAllowed(client.addr, m_daemon);
    m_map[client.addr] = allowed;
    return allowed;
#else
    return true;
#endif
}

} // namespace util

#ifdef TEST

int main()
{
    util::IPAddress addr = util::IPAddress::FromDottedQuad(10,35,1,39);
    TRACE << ::IsAllowed(addr.addr, "choraled") << "\n";
    TRACE << ::IsAllowed(addr.addr, "choraled.ro") << "\n";
}

#endif
