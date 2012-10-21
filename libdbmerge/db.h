#ifndef LIBDBMERGE_DB_H
#define LIBDBMERGE_DB_H

#include "libmediadb/db.h"

namespace util { class SeekableStream; }

namespace db {

/** A database representing a merged view of a collection of other databases.
 *
 * Although the design allows this to be arbitrarily complicated (eg
 * collating artists across all the databases, and/or merging
 * similarly-named folders), all that's implemented initially is a
 * combined root-directory view.
 *
 * IDs from the various databases are distinguished by the top 8 bits, so that
 * for instance ID 0x120 from database 6 is presented to clients as 0x06000120.
 */
namespace merge {

class Database: public mediadb::Database
{
    class Impl;

    Impl *m_impl;

    class Query;
    class RootRecordset;
    class WrapRecordset;

public:
    Database();
    ~Database();

    unsigned int AddDatabase(mediadb::Database*);
    unsigned int RemoveDatabase(mediadb::Database*);

    // Being a db::Database
    RecordsetPtr CreateRecordset();
    QueryPtr CreateQuery();

    // Being a mediadb::Database
    unsigned int AllocateID();
    std::string GetURL(unsigned int id);
    std::auto_ptr<util::Stream> OpenRead(unsigned int id);
    std::auto_ptr<util::Stream> OpenWrite(unsigned int id);
};

} // namespace db::merge
} // namespace db

#endif
