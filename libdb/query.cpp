#include "db.h"
#include <sstream>

namespace db {

Query::Query() 
    : m_root(0)
{
}

Query::~Query() {}

const Query::Rep *Query::Restrict(int which, RestrictionType rt,
				  const std::string& val)
{
    m_restrictions.push_back(Restriction(which, rt, val));
    return (const Query::Rep*)m_restrictions.size();
}

const Query::Rep *Query::Restrict(int which, RestrictionType rt,
				  uint32_t val)
{
    m_restrictions.push_back(Restriction(which, rt, val));
    return (const Query::Rep*)m_restrictions.size();
}

const Query::Rep *Query::And(const Rep *a, const Rep *b)
{
    ssize_t sa = (ssize_t)a;
    ssize_t sb = (ssize_t)b;

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
    return (const Query::Rep*)-m_relations.size();
}

const Query::Rep *Query::Or(const Rep *a, const Rep *b)
{
    ssize_t sa = (ssize_t)a;
    ssize_t sb = (ssize_t)b;

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
    return (const Query::Rep*)-m_relations.size();
}

/** Apply a condition to the query (like SQL "SELECT * WHERE ...")
 *
 * The 'Rep' isn't actually a pointer -- that's a pretence to keep
 * client code honest. In fact, it's a ssize_t; positive values refer
 * to individual restrictions (leaf nodes of the syntax tree) in the
 * m_restrictions array, and negative values refer to ands and ors
 * (internal nodes of the syntax tree) in the m_relations array.
 */
unsigned int Query::Where(const Rep *expr)
{
    m_root = (ssize_t)expr;

    // Can be 0, meaning null query

    if (m_root > 0)
	assert(m_root <= (ssize_t)m_restrictions.size());
    else if (m_root < 0)
	assert(-m_root <= (ssize_t)m_relations.size());

    return 0;
}

/** Impose a sort order on the results (like SQL "ORDER BY ...")
 */
unsigned int Query::OrderBy(int which)
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
unsigned int Query::CollateBy(int which)
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
	const Restriction& r = m_restrictions[elem-1];
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
	const Relation& r = m_relations[-elem-1];
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
