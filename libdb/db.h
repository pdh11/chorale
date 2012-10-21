/* db/db.h
 *
 * Generic database API, implemented by SteamDB and others.
 */

#ifndef DB_DB_H
#define DB_DB_H

#include <stdint.h>
#include <boost/shared_ptr.hpp>

/** Interface classes for a generic database abstraction.
 *
 * Implementations of this interface can be found in db::steam and elsewhere.
 */
namespace db {

/** Models a cursor into the results of a query.
 *
 * Note that there is no separate class for a "Record" -- as all records are
 * accessed via cursors, it's simpler to elide cursor->record->getfield into
 * just cursor->getfield.
 */
class Recordset
{
public:
    virtual ~Recordset() {}
    virtual bool IsEOF() = 0;

    virtual uint32_t GetInteger(int which) = 0;
    virtual std::string GetString(int which) = 0;

    virtual unsigned int SetInteger(int which, uint32_t value) = 0;
    virtual unsigned int SetString(int which, const std::string& value) = 0;

    virtual void MoveNext() = 0;
    virtual unsigned int AddRecord() = 0;
    virtual unsigned int Commit() = 0;

    /** Deletes current record, if any. Implicitly does MoveNext.
     */
    virtual unsigned int Delete() = 0;
};

typedef ::boost::shared_ptr<Recordset> RecordsetPtr;

enum RestrictionType 
{
    EQ,
    NE,
    GT,
    LIKE
};

/** Models a database query.
 *
 * Currently all restrictions are AND'd together; there's no way of doing OR.
 */
class Query
{
public:
    virtual ~Query() {}
    virtual unsigned int Restrict(int which, RestrictionType rt, 
				  const std::string& val) = 0;
    virtual unsigned int Restrict(int which, RestrictionType rt,
				  uint32_t val) = 0;
    virtual unsigned int OrderBy(int which) = 0;
    virtual unsigned int CollateBy(int which) = 0;

    virtual RecordsetPtr Execute() = 0;
};

typedef ::boost::shared_ptr<Query> QueryPtr;

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

typedef ::boost::shared_ptr<Database> DatabasePtr;

}; // namespace db

#endif
