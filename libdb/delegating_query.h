#ifndef LIBDB_DELEGATING_QUERY_H
#define LIBDB_DELEGATING_QUERY_H 1

#include "db.h"

namespace db {

/** A class that does nothing but pass Query operations on to another.
 *
 * This is useful to derive from when only overriding one or two operations.
 */
class DelegatingQuery: public Query
{
protected:
    db::QueryPtr m_qp;

public:
    explicit DelegatingQuery(db::QueryPtr qp) : m_qp(qp) {}

    // Being a Query
    Subexpression Restrict(field_t which, RestrictionType rt, 
			   const std::string& val);
    Subexpression Restrict(field_t which, RestrictionType rt, uint32_t val);
    Subexpression And(const Subexpression&, const Subexpression&);
    Subexpression Or(const Subexpression&, const Subexpression&);
    unsigned int Where(const Subexpression&);
    unsigned int OrderBy(field_t which);
    unsigned int CollateBy(field_t which);
    RecordsetPtr Execute();
};

} // namespace db

#endif
