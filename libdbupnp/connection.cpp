#include "connection.h"
#include "libutil/trace.h"
#include "libupnp/ssdp.h"
#include "libmediadb/schema.h"
#include <string.h>

namespace db {
namespace upnp {

Connection::Connection(util::http::Client *client,
		       util::http::Server *server,
		       util::Scheduler *poller)
    : m_device_client(client, server, poller),
      m_contentdirectory(&m_device_client, 
			 ::upnp::s_service_id_content_directory),
      m_msmediareceiver(&m_device_client,
			::upnp::s_service_id_ms_receiver_registrar, this),
      m_nextid(0x110),
      m_search_caps(0),
      m_forbidden(false)
{
}

Connection::~Connection()
{
}

unsigned Connection::Init(const std::string& url, const std::string& udn,
			  InitCallback callback)
{
    TRACE << "Async init calls DCI\n";
    m_callback = callback;
    return m_device_client.Init(url, udn, 
				NextCallback<&Connection::OnDeviceInitialised>());
}

unsigned Connection::OnDeviceInitialised(unsigned int rc)
{
    TRACE << "DCI returned, calling MRI\n";
    if (rc)
	return m_callback(rc);

    rc = m_msmediareceiver.Init(NextCallback<&Connection::OnMSInitialised>());
    if (!rc)
	return 0;

    // Not Microsoft
    OnIsAuthorizedDone(0, true);
    return 0;
}

unsigned Connection::OnMSInitialised(unsigned int rc)
{
    TRACE << "MRI returned " << rc << ", calling CDI\n";

    if (!rc)
	return m_msmediareceiver.IsAuthorized("");

    return m_contentdirectory.Init(NextCallback<&Connection::OnCDSInitialised>());
}

void Connection::OnIsAuthorizedDone(unsigned rc, bool whether)
{
    TRACE << "MSIA returned " << rc << ", " << whether << "\n";
    m_forbidden = !whether;
    m_contentdirectory.Init(NextCallback<&Connection::OnCDSInitialised>());
}

unsigned Connection::OnCDSInitialised(unsigned int rc)
{
    TRACE << "CDI returned, calling GSC next\n";
    if (rc && m_callback.IsValid())
	return m_callback(rc);
    
    {
	Lock lock(this);

	// Set up root item
	m_idmap[0x100] = "0";
	m_revidmap["0"] = 0x100;
	m_nextid = 0x110;
    }

    /** @todo Work out how to do this async, but go sync later on
     *
     * Have both async and sync methods on same class?
     * Have SyncAPI/AsyncAPI/Client as three separate objects?
     * Stay async and make database API async?
     */

    std::string sc;
    rc = m_contentdirectory.GetSearchCapabilities(&sc);
    if (rc == 0)
    {
	TRACE << "SearchCaps='" << sc << "'\n";
    }
    else
	TRACE << "Can't GetSearchCapabilities: " << rc << "\n";

    if (sc == "*" || strstr(sc.c_str(), "upnp:class"))
    {
	std::string result;
	rc = m_contentdirectory.Browse("0",
				       ::upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA,
				       "upnp:searchClass", 0, 0, "",
				       &result, NULL, NULL, NULL);
	if (rc == 0)
	{
	    TRACE << "searchClasses=" << result << "\n";
	    
	    if (strstr(result.c_str(), "object.container.person.musicArtist")
		&& (sc == "*" || strstr(sc.c_str(), "upnp:artist")))
		m_search_caps |= (1<<mediadb::ARTIST);
	    if (strstr(result.c_str(), "object.container.album.musicAlbum")
		&& (sc == "*" || strstr(sc.c_str(), "upnp:album")))
		m_search_caps |= (1<<mediadb::ALBUM);
	    if (strstr(result.c_str(), "object.container.genre.musicGenre")
		&& (sc == "*" || strstr(sc.c_str(), "upnp:genre")))
		m_search_caps |= (1<<mediadb::GENRE);
	}
	else
	{
	    TRACE << "Can't Browse root item: " << rc << "\n";
	}
    }

    if (m_callback.IsValid())
	return m_callback(0);
    return 0;
}

unsigned Connection::Init(const std::string& url, const std::string& udn)
{
    TRACE << "Sync init\n";
    unsigned int rc = m_device_client.Init(url, udn);
    if (rc)
	return rc;

    rc = m_contentdirectory.Init();
    if (rc)
	return rc;

    TRACE << "Sync init done\n";
    return OnCDSInitialised(0);
}

const std::string& Connection::GetFriendlyName() const
{
    return m_device_client.GetFriendlyName();
}

unsigned int Connection::IdForObjectId(const std::string& objectid)
{
    Lock lock(this);

    revidmap_t::const_iterator it = m_revidmap.find(objectid);
    if (it != m_revidmap.end())
	return it->second;

    unsigned int id = m_nextid++;
    m_idmap[id] = objectid;
    m_revidmap[objectid] = id;
    return id;
}

std::string Connection::ObjectIdForId(unsigned int id)
{
    Lock lock(this);

    idmap_t::const_iterator it = m_idmap.find(id);
    if (it != m_idmap.end())
	return it->second;

    // This way round should never be the first time we ask the question

    TRACE << "Unknown id " << id << "\n";
    return std::string();
}

} // namespace db::upnp
} // namespace db
