/* libdb/query_impl.h */
#ifndef LIBDB_QUERY_IMPL_H
#define LIBDB_QUERY_IMPL_H 1

#include <string>
#include <list>
#include "db.h"

namespace db {

/** A class that implements most of being a query.
 *
 * Just override Execute as required.
 *
 * Note that Restrict, OrderBy, and Collate *always* succeed on this
 * Query implementation. If yours sometimes don't, override them, make
 * your checks, then call the base-class (QueryImpl) version.
 */
class QueryImpl: public Query
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

    typedef std::list<Restriction> restrictions_t;
    typedef std::list<int> orderby_t;

protected:
    restrictions_t m_restrictions;
    orderby_t m_orderby;
    orderby_t m_collateby;

    std::string ToString();

public:
    // Being a Query
    unsigned int Restrict(int which, RestrictionType rt,
			  const std::string& val);
    unsigned int Restrict(int which, RestrictionType rt, uint32_t val);
    unsigned int OrderBy(int which);
    unsigned int CollateBy(int which);

    // Execute left unimplemented
};

};

#endif
