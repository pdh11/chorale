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

typedef unsigned int field_t;

/** Models a cursor into the results of a query.
 *
 * Note that there is no separate class for a "Record" -- as all records are
 * accessed via cursors, it's simpler to elide cursor->record->getfield into
 * just cursor->getfield.
 */
class Recordset: public util::CountedObject<>
{
public:
    virtual ~Recordset() {}
    virtual bool IsEOF() = 0;

    virtual uint32_t GetInteger(field_t which) = 0;
    virtual std::string GetString(field_t which) = 0;

    virtual unsigned int SetInteger(field_t which, uint32_t value) = 0;
    virtual unsigned int SetString(field_t which,
				   const std::string& value) = 0;

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
class Query: public util::CountedObject<>
{
public:
    struct Restriction {
	field_t which;
	RestrictionType rt;
	bool is_string;
	std::string sval;
	uint32_t ival;

	Restriction(field_t w, RestrictionType r, const std::string& val)
	    : which(w), rt(r), is_string(true), sval(val) {}

	Restriction(field_t w, RestrictionType r, uint32_t val)
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

    typedef std::list<field_t> orderby_t;

protected:
    restrictions_t m_restrictions;
    relations_t m_relations;
    int m_root;
    orderby_t m_orderby;
    orderby_t m_collateby;

    std::string ToStringElement(ssize_t) const;

public:
    Query();
    virtual ~Query();

    class Subexpression
    {
	int val;
	explicit Subexpression(int v) : val(v) {}
	friend class Query;

    public:
	Subexpression() : val(0) {}
	bool IsValid() const { return val != 0; }
    };

    virtual Subexpression Restrict(field_t which, RestrictionType rt, 
				   const std::string& val)
	ATTRIBUTE_WARNUNUSED;
    virtual Subexpression Restrict(field_t which, RestrictionType rt,
				   uint32_t val) ATTRIBUTE_WARNUNUSED;
    virtual Subexpression And(const Subexpression&, const Subexpression&);
    virtual Subexpression Or(const Subexpression&, const Subexpression&);

    virtual unsigned int Where(const Subexpression&);
    virtual unsigned int OrderBy(field_t which);
    virtual unsigned int CollateBy(field_t which);

    virtual RecordsetPtr Execute() = 0;

    std::string ToString() const;

    /** Copies the Where, OrderBy, and CollateBy information from
     * another query.
     *
     * Useful for applying the same query to more than one
     * Database. Note that this operation may fail: the target
     * database's Where, OrderBy, or Collate may refuse it. (That's
     * why this operation isn't just a copy of m_restrictions,
     * m_relations etc.)
     */
    unsigned int Clone(const Query *other);
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
