#ifndef CHORALED_NFS_H
#define CHORALED_NFS_H 1

namespace util { class Scheduler; }
namespace util { class IPFilter; }

namespace choraled {

/** An NFS server (and associated other services) used for booting Rio
 * Receivers.
 */
class NFSService
{
    class Impl;
    Impl *m_impl;
public:
    NFSService();

    unsigned int Init(util::Scheduler *poller, util::IPFilter *filter,
		      const char *arf);

    unsigned short GetPort();

    ~NFSService();
};

} // namespace choraled

#endif
