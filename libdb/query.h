#ifndef LIBDB_QUERY_H
#define LIBDB_QUERY_H

#include <vector>
#include <list>
#include <string>
#include <stdint.h>
#include "libutil/counted_object.h"
#include "libutil/attributes.h"
#include "db.h"
#include "recordset.h"

namespace db {

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
	unsigned int which;
	RestrictionType rt;
	bool is_string;
	std::string sval;
	uint32_t ival;

	Restriction(unsigned int w, RestrictionType r, const std::string& val)
	    : which(w), rt(r), is_string(true), sval(val) {}

	Restriction(unsigned int w, RestrictionType r, uint32_t val)
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

    typedef std::list<unsigned int> orderby_t;

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

    virtual Subexpression Restrict(unsigned int which, RestrictionType rt, 
				   const std::string& val)
	ATTRIBUTE_WARNUNUSED;
    virtual Subexpression Restrict(unsigned int which, RestrictionType rt,
				   uint32_t val) ATTRIBUTE_WARNUNUSED;
    virtual Subexpression And(const Subexpression&, const Subexpression&);
    virtual Subexpression Or(const Subexpression&, const Subexpression&);

    virtual unsigned int Where(const Subexpression&);
    virtual unsigned int OrderBy(unsigned int which);
    virtual unsigned int CollateBy(unsigned int which);

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

} // namespace db

#endif
