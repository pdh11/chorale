#ifndef LIBDBUPNP_CONNECTION_H
#define LIBDBUPNP_CONNECTION_H 1

#include "libutil/bind.h"
#include "libupnp/client.h"
#include "libupnp/ContentDirectory_client.h"
#include "libupnp/MSMediaReceiverRegistrar_client.h"
#include <map>

namespace db {
namespace upnp {

/** Connection to a UPNP database server
 *
 * The lock protects (at least) m_nextid, m_idmap, m_revidmap, and m_infomap.
 *
 *
 * The problem of idmap/revidmap lifetime
 * --------------------------------------
 *
 * Servers are allowed to use arbitrary strings as ObjectIDs. But
 * clients of db::Database are entitled to believe that a mediadb::ID
 * (an integer) is enough to uniquely identify a record. So we need to
 * keep a map. How long for? Well, sadly, we don't know. The
 * mediadb::IDs could go to Rio Receivers, be kept there, and come
 * back again. For the time being, we keep them indefinitely; this is,
 * effectively, a memory leak. Embedded users will probably want a way of
 * clearing the maps (e.g. on select-whole-new-playlist).
 *
 * @todo Get rid of all of these friend declarations.
 */
class Connection: public util::PerObjectRecursiveLocking,
    public ::upnp::MSMediaReceiverRegistrarAsyncObserver
{
    ::upnp::DeviceClient m_device_client;
    ::upnp::ContentDirectoryClient m_contentdirectory;
    ::upnp::MSMediaReceiverRegistrarClientAsync m_msmediareceiver;

    typedef std::map<unsigned int, std::string> idmap_t;
    idmap_t m_idmap;
    typedef std::map<std::string, unsigned int> revidmap_t;
    revidmap_t m_revidmap;
    unsigned int m_nextid;

    unsigned int IdForObjectId(const std::string& objectid);
    
    std::string ObjectIdForId(unsigned int id);

    struct BasicInfo
    {
	unsigned int type;
	std::string title;
    };
    
    typedef std::map<unsigned int, BasicInfo> infomap_t;
    infomap_t m_infomap;

    friend class Query;
    friend class Recordset;
    friend class RecordsetOne;
    friend class SearchRecordset;
    friend class CollateRecordset;

    /** Bit N set => can search by field N
     *
     * eg "can_search_artists = m_search_caps & (1<<mediadb::ARTIST)"
     */
    unsigned int m_search_caps;

    /** Access is forbidden (e.g. by Windows Media Connect)
     */
    bool m_forbidden;

    typedef util::Callback1<unsigned int> InitCallback;
    
    InitCallback m_callback;

    template <unsigned int (Connection::*FN)(unsigned int)>
    InitCallback NextCallback()
    {
	return util::Bind(this).To<unsigned int, FN>();
    }

    unsigned OnDeviceInitialised(unsigned);
    unsigned OnCDSInitialised(unsigned);
    unsigned OnMSInitialised(unsigned);
    void OnIsAuthorizedDone(unsigned rc, bool whether);
    
public:
    Connection(util::http::Client*, util::http::Server*, util::Scheduler*);
    ~Connection();

    unsigned Init(const std::string& url, const std::string& udn);
    unsigned Init(const std::string& url, const std::string& udn,
		  InitCallback cb);

    unsigned int AllocateID() { return m_nextid++; }

    ::upnp::ContentDirectory *GetContentDirectory()
    {
	return &m_contentdirectory;
    }

    const std::string& GetFriendlyName() const;

    bool IsForbidden() const { return m_forbidden; }
};

} // namespace db::upnp
} // namespace db

#endif
