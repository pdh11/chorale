#ifndef MEDIADB_LOCALDB_H
#define MEDIADB_LOCALDB_H 1

#include <map>
#include "db.h"

namespace mediadb {

/** A factory for generating streams given a database entry.
 *
 * LocalDatabase allows a StreamFactory to be registered per record type
 * (i.e. per value of the mediadb::TYPE field).
 *
 * This is used to implement streaming radio support for type
 * mediadb::RADIO.
 */
class StreamFactory
{
public:
    virtual ~StreamFactory() {}
    
    virtual std::string GetURL(db::RecordsetPtr rs) = 0;
    virtual util::SeekableStreamPtr OpenRead(db::RecordsetPtr rs) = 0;
    virtual util::SeekableStreamPtr OpenWrite(db::RecordsetPtr rs) = 0;
};

/** A mediadb::Database that wraps up a db::Database and accesses data via
 * its "path" field.
 */
class LocalDatabase: public Database
{
    db::Database *m_db;
    typedef std::map<unsigned int, StreamFactory*> factories_t;
    factories_t m_factories;

public:
    explicit LocalDatabase(db::Database *thedb) : m_db(thedb) {}
    ~LocalDatabase() {}

    db::RecordsetPtr CreateRecordset();
    db::QueryPtr CreateQuery();
    std::string GetURL(unsigned int id);
    util::SeekableStreamPtr OpenRead(unsigned int id);
    util::SeekableStreamPtr OpenWrite(unsigned int id);

    void RegisterStreamFactory(unsigned int type, StreamFactory *f)
    {
	m_factories[type] = f;
    }
};

} // namespace mediadb

#endif
