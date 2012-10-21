#include "ip_filter.h"

namespace choraled {

IPFilter::IPFilter()
    : m_wrappers("choraled"),
      m_wrappers_readonly("choraled.ro")
{
}

IPFilter::~IPFilter()
{
}

unsigned int IPFilter::CheckAccess(util::IPAddress ip)
{
    if (m_wrappers.Allowed(ip))
	return FULL;
    if (m_wrappers_readonly.Allowed(ip))
	return READONLY;
    return DENY;
}

} // namespace choraled
