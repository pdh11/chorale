#ifndef UPNPD_CONNECTION_MANAGER_H
#define UPNPD_CONNECTION_MANAGER_H

#include "libupnp/ConnectionManager.h"

namespace upnpd {

/** Actual implementation of upnp::ConnectionManager base class.
 *
 * Bare minimum required by standard.
 */
class ConnectionManagerImpl: public upnp::ConnectionManager
{
public:
    enum WhichEnd { CLIENT, SERVER };

private:
    WhichEnd m_end;
    std::string m_protocol_info;

public:
    ConnectionManagerImpl(WhichEnd which_end,
			  const std::string& protocol_info)
	: m_end(which_end),
	  m_protocol_info(protocol_info)
    {
    }

    // Being a ConnectionManager2
    unsigned int GetCurrentConnectionIDs(std::string *connection_i_ds);
    unsigned int GetCurrentConnectionInfo(int32_t connection_id,
					  int32_t *rcs_id,
					  int32_t *av_transport_id,
					  std::string *protocol_info,
					  std::string *peer_connection_manager,
					  int32_t *peer_connection_id,
					  Direction *direction,
					  ConnectionStatus *status);
    unsigned int GetProtocolInfo(std::string *source,
				 std::string *sink);
};

} // namespace upnpd

#endif
