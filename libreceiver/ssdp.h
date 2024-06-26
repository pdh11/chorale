/* ssdp.h */

#ifndef LIBRECEIVER_SSDP_H
#define LIBRECEIVER_SSDP_H 1

#include <boost/utility.hpp>
#include "libutil/counted_pointer.h"

namespace util { struct IPEndPoint; }
namespace util { class Scheduler; }
namespace util { class IPFilter; }

namespace receiver {

/** Classes for Empeg-style pseudo-SSDP as used by Rio Receiver.
 *
 * It's not "real" SSDP, but it does a similar job and is based on a very
 * simplified SSDP-style protocol.
 */
namespace ssdp {

/** Empeg-style pseudo-SSDP server, advertises services on the network.
 */
class Server: public boost::noncopyable
{
    class Task;
    typedef util::CountedPointer<Task> TaskPtr;
    TaskPtr m_task;

public:
    explicit Server(util::IPFilter*);
    ~Server();

    unsigned Init(util::Scheduler*);

    /** Advertise a service on the network.
     *
     * Note that the Rio Receiver doesn't deal with more than one
     * server per service per (local) network, but that's a limitation
     * of the client, not the protocol.
     *
     * Passing NULL as service_host (the default) means "this host".
     */
    void RegisterService(const char *uuid, unsigned short service_port,
			 const char *service_host = NULL);
};

/** Empeg-style pseudo-SSDP client, looks for services on the network.
 *
 * Calls you back on the Scheduler's thread, whichever that is.
 *
 * Because the wire protocol is so limited, you need a Client per service.
 */
class Client: public boost::noncopyable
{
    class Task;
    typedef util::CountedPointer<Task> TaskPtr;
    TaskPtr m_task;

public:
    Client();
    ~Client();

    class Callback
    {
    public:
	virtual ~Callback() {}

	virtual void OnService(const util::IPEndPoint&) = 0;
    };

    unsigned Init(util::Scheduler*, const char *uuid, Callback*);
};

/** UUID for the Rio Receiver software (NFS) service.
 */
extern const char s_uuid_softwareserver[];

/** UUID for the Rio Receiver music (HTTP) service.
 */
extern const char s_uuid_musicserver[];

} // namespace ssdp

} // namespace receiver

#endif
