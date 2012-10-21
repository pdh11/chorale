/* libdbupnp/db.h */
#ifndef LIBDBUPNP_DB_H
#define LIBDBUPNP_DB_H

#include <string>
#include <map>
#include "libutil/stream.h"
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
 */
class Database: public mediadb::Database
{
    upnp::Client m_upnp;
    upnp::ContentDirectory2Client m_contentdirectory;

    typedef std::map<unsigned int, std::string> idmap_t;
    idmap_t m_idmap;
    typedef std::map<std::string, unsigned int> revidmap_t;
    revidmap_t m_revidmap;
    unsigned int m_nextid;

    unsigned int IdForObjectId(const std::string& objectid);

    friend class Query;
    friend class Recordset;
    friend class RecordsetOne;

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
