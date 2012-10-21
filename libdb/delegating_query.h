#ifndef LIBDB_DELEGATING_QUERY_H
#define LIBDB_DELEGATING_QUERY_H 1

#include "query.h"
#include "libutil/counted_pointer.h"

namespace db {

/** A class that does nothing but pass Query operations on to another.
 *
 * This is useful to derive from when only overriding one or two operations.
 */
class DelegatingQuery: public Query
{
protected:
    util::CountedPointer<db::Query> m_qp;

public:
    explicit DelegatingQuery(util::CountedPointer<db::Query> qp) : m_qp(qp) {}

    // Being a Query
    Subexpression Restrict(unsigned int which, RestrictionType rt, 
			   const std::string& val);
    Subexpression Restrict(unsigned int which, RestrictionType rt,
			   uint32_t val);
    Subexpression And(const Subexpression&, const Subexpression&);
    Subexpression Or(const Subexpression&, const Subexpression&);
    unsigned int Where(const Subexpression&);
    unsigned int OrderBy(unsigned int which);
    unsigned int CollateBy(unsigned int which);
    util::CountedPointer<Recordset> Execute();
};

} // namespace db

#endif
