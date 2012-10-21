#ifndef LIBDBEMPEG_DB_H
#define LIBDBEMPEG_DB_H 1

#include "libmediadb/db.h"

namespace util { class IPAddress; }
namespace util { namespace http { class Server; } }

namespace db {

/** A database representing the files on a network-attached Empeg car-player.
 */
namespace empeg {

class Connection;

class Database: public mediadb::Database
{
    Connection *m_connection;

public:
    explicit Database(util::http::Server*);
    ~Database();

    unsigned int Init(const util::IPAddress&);

    // Being a db::Database
    db::RecordsetPtr CreateRecordset();
    db::QueryPtr CreateQuery();

    // Being a mediadb::Database
    unsigned int AllocateID();
    std::string GetURL(unsigned int id);
    std::auto_ptr<util::Stream> OpenRead(unsigned int id);
    std::auto_ptr<util::Stream> OpenWrite(unsigned int id);
};

} // namespace db::empeg

} // namespace db

#endif
