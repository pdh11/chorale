#include "db.h"
#include <sstream>

namespace db {

Query::Query() 
    : m_root(0)
{
}

Query::~Query() {}

Query::Subexpression Query::Restrict(field_t which, RestrictionType rt,
				     const std::string& val)
{
    m_restrictions.push_back(Restriction(which, rt, val));
    return Subexpression((int)m_restrictions.size());
}

Query::Subexpression Query::Restrict(field_t which, RestrictionType rt,
				     uint32_t val)
{
    m_restrictions.push_back(Restriction(which, rt, val));
    return Subexpression((int)m_restrictions.size());
}

Query::Subexpression Query::And(const Subexpression& a, const Subexpression& b)
{
    int sa = a.val;
    int sb = b.val;

    assert(sa != 0);
    assert(sb != 0);

    if (sa > 0)
	assert(sa <= (ssize_t)m_restrictions.size());
    else
	assert(-sa <= (ssize_t)m_relations.size());
    if (sb > 0)
	assert(sb <= (ssize_t)m_restrictions.size());
    else
	assert(-sb <= (ssize_t)m_relations.size());

    m_relations.push_back(Relation(true, sa, sb));
    return Subexpression((int) -m_relations.size());
}

Query::Subexpression Query::Or(const Subexpression& a, const Subexpression& b)
{
    int sa = a.val;
    int sb = b.val;

    assert(sa != 0);
    assert(sb != 0);

    if (sa > 0)
	assert(sa <= (ssize_t)m_restrictions.size());
    else
	assert(-sa <= (ssize_t)m_relations.size());
    if (sb > 0)
	assert(sb <= (ssize_t)m_restrictions.size());
    else
	assert(-sb <= (ssize_t)m_relations.size());

    m_relations.push_back(Relation(false, sa, sb));
    return Subexpression((int) -m_relations.size());
}

/** Apply a condition to the query (like SQL "SELECT * WHERE ...")
 */
unsigned int Query::Where(const Subexpression& expr)
{
    m_root = expr.val;

    // Can be 0, meaning null query

    if (m_root > 0)
	assert(m_root <= (ssize_t)m_restrictions.size());
    else if (m_root < 0)
	assert(-m_root <= (ssize_t)m_relations.size());

    return 0;
}

/** Impose a sort order on the results (like SQL "ORDER BY ...")
 */
unsigned int Query::OrderBy(field_t which)
{
    m_orderby.push_back(which);
    return 0;
}

/** Do a collation query (like SQL "GROUP BY ...")
 *
 * This is unlike the other options, in that the recordset returned has a
 * different schema. Field 0 is the first collation field, field 1 the second,
 * and so on.
 */
unsigned int Query::CollateBy(field_t which)
{
    m_collateby.push_back(which);
    return 0;
}

std::string Query::ToString()
{
    std::ostringstream os;
    os << ToStringElement(m_root);

    for (orderby_t::const_iterator i = m_orderby.begin();
         i != m_orderby.end();
         ++i)
        os << "order by #" << *i << " ";

    for (orderby_t::const_iterator i = m_collateby.begin();
         i != m_collateby.end();
         ++i)
        os << "collate by #" << *i << " ";

    return os.str();
}

std::string Query::ToStringElement(ssize_t elem)
{
    if (elem == 0)
	return "";

    if (elem > 0)
    {
	const Restriction& r = m_restrictions[(size_t)(elem-1)];
	std::ostringstream os;
	os << "#" << r.which << " ";
	switch (r.rt)
	{
	case EQ:
	    os << "=";
	    break;
	case NE:
	    os << "!=";
	    break;
	case GT:
	    os << ">";
	    break;
	case GE:
	    os << ">=";
	    break;
	case LT:
	    os << "<";
	    break;
	case LE:
	    os << "<=";
	    break;
	case LIKE:
	    os << "?=";
	    break;
	default:
	    assert(false);
	    break;
	}
	os << " ";
	if (r.is_string)
	    os << "\"" + r.sval + "\"";
	else
	    os << r.ival;
	os << " ";
	return os.str();
    }
    else
    {
	const Relation& r = m_relations[(size_t)(-elem-1)];
	std::ostringstream os;
	
	os << "( " << ToStringElement(r.a);
	if (r.anditive)
	    os << "and ";
	else
	    os << "or ";
	os << ToStringElement(r.b) << ") ";
	return os.str();
    }
}

} // namespace db
