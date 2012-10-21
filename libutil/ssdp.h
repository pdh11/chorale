/* libutil/ssdp.h */

#ifndef LIBUTIL_SSDP_H
#define LIBUTIL_SSDP_H 1

#include <string>

namespace util {

class PollerInterface;

/** Classes implementing SSDP, the discovery protocol used in UPnP.
 *
 * Note that this is "real" SSDP, as opposed to the pseudo-SSDP implemented
 * by receiver::ssdp.
 */
namespace ssdp {

class Client
{
    class Impl;
    Impl *m_impl;

public:
    Client(util::PollerInterface*);
    ~Client();

    class Callback
    {
    public:
	virtual ~Callback() {}

	virtual void OnService(const std::string& descurl,
			       const std::string& udn) = 0;
    };

    unsigned Init(const char *uuid, Callback*);
};

extern const char *const s_uuid_contentdirectory;
extern const char *const s_uuid_avtransport;
extern const char *const s_uuid_connectionmanager;
extern const char *const s_uuid_opticaldrive;

} // namespace ssdp

} // namespace util

#endif
