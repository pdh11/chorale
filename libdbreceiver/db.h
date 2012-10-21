#ifndef LIBDBRECEIVER_DB_H
#define LIBDBRECEIVER_DB_H 1

#include "libmediadb/db.h"
#include "libutil/socket.h"
#include <set>
#include <map>

namespace util { namespace http { class Client; } }

namespace db {

/** A database representing the files on a remote Rio Receiver server.
 *
 * In other words, this is Chorale's implementation of the client end of the
 * Rio Receiver protocol, for which see http://www.dasnet.org/node/69
 */
namespace receiver {

class Recordset;
class CollateRecordset;
class RestrictionRecordset;

/** Implementation of db::Database; also owns the connection to the server.
 */
class Database: public mediadb::Database
{
    util::http::Client *m_http;
    util::IPEndPoint m_ep;
    bool m_got_tags;

    std::set<int> m_has_tags;
    std::map<int, int> m_server_to_media_map;
    std::map<int, std::string> m_tag_to_fieldname_map;

    bool HasTag(int which);

//    std::map<int, int> m_id_to_type_map;
//    std::map<int, std::string> m_id_to_title_map;

    friend class Recordset;
    friend class CollateRecordset;
    friend class RestrictionRecordset;

public:
    explicit Database(util::http::Client *http);

    unsigned int Init(const util::IPEndPoint&);

    std::string GetFieldName(int mediadbtag) { return m_tag_to_fieldname_map[mediadbtag]; }

    // Being a db::Database
    db::RecordsetPtr CreateRecordset();
    db::QueryPtr CreateQuery();

    // Being a mediadb::Database
    std::string GetURL(unsigned int id);
    util::SeekableStreamPtr OpenRead(unsigned int id);
    util::SeekableStreamPtr OpenWrite(unsigned int id);
};

} // namespace receiver
} // namespace db

#endif
