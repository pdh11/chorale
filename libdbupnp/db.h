/* libdbupnp/db.h */
#ifndef LIBDBUPNP_DB_H
#define LIBDBUPNP_DB_H

#include <string>
#include <map>
#include "libutil/stream.h"
#include "libmediadb/db.h"

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
    std::string m_udn;
    std::string m_description_url;
    std::string m_control_url;
    std::string m_presentation_url;
    std::string m_friendly_name;
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
    Database() {}

    unsigned Init(const std::string& url);
    const std::string& GetFriendlyName() const { return m_friendly_name; }

    // Being a Database
    RecordsetPtr CreateRecordset();
    QueryPtr CreateQuery();

    // Being a mediadb::Database
    std::string GetURL(unsigned int id);
    util::SeekableStreamPtr OpenRead(unsigned int id);
    util::SeekableStreamPtr OpenWrite(unsigned int id);
};

}; // namespace upnpav
}; // namespace db

#endif
