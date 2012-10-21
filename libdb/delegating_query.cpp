#include "delegating_query.h"
#include "recordset.h"

namespace db {

Query::Subexpression DelegatingQuery::Restrict(unsigned int which, 
					       RestrictionType rt, 
					       const std::string& val)
{
    return m_qp->Restrict(which, rt, val);
}

Query::Subexpression DelegatingQuery::Restrict(unsigned int which,
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

unsigned int DelegatingQuery::OrderBy(unsigned int which)
{
    return m_qp->OrderBy(which);
}

unsigned int DelegatingQuery::CollateBy(unsigned int which)
{
    return m_qp->CollateBy(which);
}

util::CountedPointer<Recordset> DelegatingQuery::Execute()
{
    return m_qp->Execute();
}

} // namespace db
