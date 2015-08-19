#ifndef CHORALED_IP_FILTER_H
#define CHORALED_IP_FILTER_H 1

#include "libutil/ip_filter.h"
#include "libutil/tcp_wrappers.h"

namespace choraled {

class IPFilter final: public util::IPFilter
{
    util::TcpWrappers m_wrappers;
    util::TcpWrappers m_wrappers_readonly;

public:
    IPFilter();
    ~IPFilter();

    unsigned int CheckAccess(util::IPAddress ip) override;
};

} // namespace choraled

#endif
