#include "delegating_query.h"

namespace db {

Query::Subexpression DelegatingQuery::Restrict(field_t which, 
					       RestrictionType rt, 
					       const std::string& val)
{
    return m_qp->Restrict(which, rt, val);
}

Query::Subexpression DelegatingQuery::Restrict(field_t which,
					       RestrictionType rt,
					       uint32_t val)
{
    return m_qp->Restrict(which, rt, val);
}

Query::Subexpression DelegatingQuery::And(const Subexpression& a,
					  const Subexpression& b)
{
    return m_qp->And(a,b);
}

Query::Subexpression DelegatingQuery::Or(const Subexpression& a, 
					 const Subexpression& b)
{
    return m_qp->Or(a,b);
}

unsigned int DelegatingQuery::Where(const Subexpression& s)
{
    return m_qp->Where(s);
}

unsigned int DelegatingQuery::OrderBy(field_t which)
{
    return m_qp->OrderBy(which);
}

unsigned int DelegatingQuery::CollateBy(field_t which)
{
    return m_qp->CollateBy(which);
}

RecordsetPtr DelegatingQuery::Execute()
{
    return m_qp->Execute();
}

} // namespace db
