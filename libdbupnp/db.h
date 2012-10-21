/* libdbupnp/db.h */
#ifndef LIBDBUPNP_DB_H
#define LIBDBUPNP_DB_H

#include <string>
#include <map>
#include "libutil/stream.h"
#include "libutil/locking.h"
#include "libmediadb/db.h"
#include "libupnp/client.h"
#include "libupnp/ContentDirectory2_client.h"

namespace db {

/** A database representing the files on a remote UPnP A/V MediaServer.
 *
 * In other words, this is Chorale's implementation of the client end of the
 * UPnP A/V MediaServer protocol.
 */
namespace upnpav {

/** Implementation of db::Database; also owns the connection to the server.
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
 */
class Database: public mediadb::Database,
		private util::PerObjectRecursiveLocking
{
    upnp::Client m_upnp;
    upnp::ContentDirectory2Client m_contentdirectory;

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

public:
    Database();
    ~Database();

    unsigned Init(const std::string& url, const std::string& udn);
    const std::string& GetFriendlyName() const;

    upnp::ContentDirectory2 *GetContentDirectory()
    {
	return &m_contentdirectory;
    }

    // Being a Database
    RecordsetPtr CreateRecordset();
    QueryPtr CreateQuery();

    // Being a mediadb::Database
    std::string GetURL(unsigned int id);
    util::SeekableStreamPtr OpenRead(unsigned int id);
    util::SeekableStreamPtr OpenWrite(unsigned int id);
};

} // namespace upnpav
} // namespace db

#endif
