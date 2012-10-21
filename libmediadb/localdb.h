#ifndef MEDIADB_LOCALDB_H
#define MEDIADB_LOCALDB_H 1

#include "db.h"

namespace mediadb {

/** A mediadb::Database that wraps up a db::Database and accesses data via
 * its "path" field.
 */
class LocalDatabase: public Database
{
    db::Database *m_db;

public:
    explicit LocalDatabase(db::Database *thedb) : m_db(thedb) {}
    ~LocalDatabase() {}

    db::RecordsetPtr CreateRecordset();
    db::QueryPtr CreateQuery();
    std::string GetURL(unsigned int id);
    util::SeekableStreamPtr OpenRead(unsigned int id);
    util::SeekableStreamPtr OpenWrite(unsigned int id);
};

}; // namespace mediadb

#endif
