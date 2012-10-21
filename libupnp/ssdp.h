/* libupnp/ssdp.h */

#ifndef LIBUPNP_SSDP_H
#define LIBUPNP_SSDP_H 1

#include <string>

namespace util { class IPFilter; }
namespace util { class PartialURL; }
namespace util { class PollerInterface; }

namespace upnp {

/** Classes implementing SSDP, the discovery protocol used in UPnP.
 *
 * Note that this is "real" SSDP, as opposed to the pseudo-SSDP implemented
 * by receiver::ssdp.
 */
namespace ssdp {

class Responder
{
    class Impl;
    Impl *m_impl;

public:
    Responder(util::PollerInterface*, util::IPFilter*);
    ~Responder();

    class Callback
    {
    public:
	virtual ~Callback() {}

	virtual void OnService(const std::string& descurl,
			       const std::string& udn) = 0;
	virtual void OnServiceLost(const std::string&, const std::string&) {}
    };

    unsigned Search(const char *uuid, Callback*);

    unsigned Advertise(const std::string& service_type,
		       const std::string& unique_device_name,
		       const util::PartialURL *url);
};


} // namespace ssdp


// Common search UUIDs

extern const char s_device_type_media_renderer[];
extern const char s_device_type_optical_drive[];
extern const char s_service_id_av_transport[];
extern const char s_service_id_connection_manager[];
extern const char s_service_id_content_directory[];
extern const char s_service_id_optical_drive[];
extern const char s_service_id_rendering_control[];
extern const char s_service_type_av_transport[];
extern const char s_service_type_connection_manager[];
extern const char s_service_type_content_directory[];
extern const char s_service_type_optical_drive[];

} // namespace upnp

#endif
