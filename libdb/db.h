/* db/db.h
 *
 * Generic database API, implemented by SteamDB and others.
 */

#ifndef DB_DB_H
#define DB_DB_H

#include <stdint.h>
#include <vector>
#include <list>
#include <string>
#include "libutil/counted_object.h"
#include "libutil/attributes.h"

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
class Recordset: public CountedObject
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

typedef ::boost::intrusive_ptr<Recordset> RecordsetPtr;

enum RestrictionType 
{
    EQ,
    NE,
    GT,
    LT,
    GE,
    LE,
    LIKE ///< Wildcard match, strings only
};

/** Models a database query.
 *
 * Note that Where, OrderBy, and Collate *always* succeed on this
 * Query implementation. If yours sometimes don't, override them, make
 * your checks, then call the base-class (Query) version.
 */
class Query: public CountedObject
{
public:
    struct Restriction {
	int which;
	RestrictionType rt;
	bool is_string;
	std::string sval;
	uint32_t ival;

	Restriction(int w, RestrictionType r, const std::string& val)
	    : which(w), rt(r), is_string(true), sval(val) {}

	Restriction(int w, RestrictionType r, uint32_t val)
	    : which(w), rt(r), is_string(false), ival(val) {}
    };
    typedef std::vector<Restriction> restrictions_t;

protected:
    struct Relation {
	bool anditive; // false=oritive
	ssize_t a;
	ssize_t b;

	Relation(bool anditive0, ssize_t a0, ssize_t b0)
	    : anditive(anditive0), a(a0), b(b0) {}
    };
    typedef std::vector<Relation> relations_t;

    typedef std::list<int> orderby_t;

protected:
    restrictions_t m_restrictions;
    relations_t m_relations;
    ssize_t m_root;
    orderby_t m_orderby;
    orderby_t m_collateby;

    std::string ToStringElement(ssize_t);

public:
    Query();
    virtual ~Query();

    class Rep;

    const Rep *Restrict(int which, RestrictionType rt, 
			const std::string& val) ATTRIBUTE_WARNUNUSED;
    const Rep *Restrict(int which, RestrictionType rt,
			uint32_t val) ATTRIBUTE_WARNUNUSED;
    const Rep *And(const Rep*, const Rep*);
    const Rep *Or(const Rep*, const Rep*);

    virtual unsigned int Where(const Rep*);
    virtual unsigned int OrderBy(int which);
    virtual unsigned int CollateBy(int which);

    virtual RecordsetPtr Execute() = 0;

    std::string ToString();
};

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
