/* db/db.h
 *
 * Generic database API, implemented by SteamDB and others.
 */

#ifndef DB_DB_H
#define DB_DB_H

#include <boost/intrusive_ptr.hpp>

/** Interface classes for a generic database abstraction.
 *
 * Implementations of this interface can be found in db::steam and elsewhere.
 */
namespace db {

typedef unsigned int field_t;

class Recordset;
typedef ::boost::intrusive_ptr<Recordset> RecordsetPtr;

class Query;
typedef ::boost::intrusive_ptr<Query> QueryPtr;

/** Models a (flatfile) database.
 *
 * The only generic operations are creating a recordset (i.e. a cursor roving
 * the whole database), and creating a query.
 */
class Database
{
public:
    virtual ~Database() {}
    virtual RecordsetPtr CreateRecordset() = 0;
    virtual QueryPtr CreateQuery() = 0;
};

} // namespace db

#endif
