#include "query_impl.h"
#include <sstream>

namespace db {

unsigned int QueryImpl::Restrict(int which, RestrictionType rt,
				 const std::string& val)
{
    m_restrictions.push_back(Restriction(which, rt, val));
    return 0;
}

unsigned int QueryImpl::Restrict(int which, RestrictionType rt, uint32_t val)
{
    m_restrictions.push_back(Restriction(which, rt, val));
    return 0;
}

unsigned int QueryImpl::OrderBy(int which)
{
    m_orderby.push_back(which);
    return 0;
}

unsigned int QueryImpl::CollateBy(int which)
{
    m_collateby.push_back(which);
    return 0;
}

std::string QueryImpl::ToString()
{
    std::ostringstream os;

    for (restrictions_t::const_iterator i=m_restrictions.begin();
	 i != m_restrictions.end();
	 ++i)
    {
	os << "#" << i->which << " ";
	switch (i->rt)
	{
	case EQ:
	    os << "=";
	    break;
	default:
	    os << "?=";
	    break;
	}
	os << " ";

	if (i->is_string)
	    os << i->sval;
	else
	    os << i->ival;
	os << " ";
    }

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

};
