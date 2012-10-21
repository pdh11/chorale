#include "connection_manager.h"

namespace upnpd {

/** List of current connections
 *
 * We don't implement PrepareForConnection, so the CM2 spec (2.4.4)
 * says return "0".
 */
unsigned ConnectionManagerImpl::GetCurrentConnectionIDs(std::string *ids)
{
    *ids = "0";
    return 0;
}

unsigned ConnectionManagerImpl::GetCurrentConnectionInfo(int32_t,
							 int32_t *rcs_id,
							 int32_t *av_transport_id,
							 std::string *protocol_info,
							 std::string *peer_connection_manager,
							 int32_t *peer_connection_id,
							 Direction *direction,
							 ConnectionStatus *status)
{
    // Again these answers are defined in CM2 spec (2.4.5)
    if (m_end == CLIENT)
    {
	*rcs_id = 0;
	*av_transport_id = 0;
	*direction = DIRECTION_INPUT;
    }
    else
    {
	*rcs_id = -1;
	*av_transport_id = -1;
	*direction = DIRECTION_OUTPUT;
    }
    *protocol_info = m_protocol_info;
    *peer_connection_manager = "";
    *peer_connection_id = -1;
    *status = CONNECTIONSTATUS_OK;
    return 0;
}

unsigned ConnectionManagerImpl::GetProtocolInfo(std::string *source,
					     std::string *sink)
{
    if (m_end == CLIENT)
    {
	*sink = m_protocol_info;
	*source = "";
    }
    else
    {
	*source = m_protocol_info;
	*sink = "";
    }
    return 0;
}

} // namespace upnpd
