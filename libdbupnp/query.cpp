#include "query.h"
#include "rs.h"
#include "db.h"
#include "libutil/trace.h"
#include "libutil/quote_escape.h"
#include "libmediadb/schema.h"
#include <boost/format.hpp>
#include <errno.h>

namespace db {
namespace upnpav {

unsigned int Query::CollateBy(field_t field)
{
    if ((m_parent->m_search_caps & (1<<field)) == 0)
	return EINVAL;
    return db::Query::CollateBy(field);
}

std::string Query::QueryElement(ssize_t elem)
{
    if (elem == 0)
	return "";

    if (elem > 0)
    {
	const Restriction& r = m_restrictions[(size_t)(elem-1)];

	const char *field = NULL;

	switch (r.which)
	{
	case mediadb::ARTIST: field = "upnp:artist"; break;
	case mediadb::ALBUM:  field = "upnp:album";  break;
	case mediadb::GENRE:  field = "upnp:genre";  break;
	}

	if (!field)
	{
	    TRACE << "Can't query on field #" << r.which << "\n";
	    return "";
	}
	
	std::string result = field;

	const char *op = NULL;
	switch (r.rt)
	{ 
	case EQ:
	    op = "=";
	    break;
	case NE:
	    op = "!=";
	    break;
	case GT:
	    op = ">";
	    break;
	case GE:
	    op = ">=";
	    break;
	case LT:
	    op = "<";
	    break;
	case LE:
	    op = "<=";
	    break;
	case LIKE:
	    op = "contains";
	    break;
	default:
	    assert(false);
	    break;
	}
	result += " ";
	result += op;
	result += " ";
	if (r.is_string)
	    result += "\"" + util::QuoteEscape(r.sval) + "\"";
	else
	    result += (boost::format("%u") % r.ival).str();
	return result;
    }
    else
    {
	const Relation& r = m_relations[(size_t)(-elem-1)];
	std::string result = "( "  + QueryElement(r.a);
	if (r.anditive)
	    result += " and ";
	else
	    result += " or ";
	result += QueryElement(r.b) + ")";
	return result;
    }
}

RecordsetPtr Query::Execute()
{
//    TRACE << "UPNP query: " << ToString() << "\n";

    if (m_collateby.size() == 1)
	return RecordsetPtr(new CollateRecordset(m_parent,
						 m_collateby.front()));

    if (m_collateby.empty())
    {
	if (m_restrictions.size() == 1
	    && m_restrictions.begin()->which == mediadb::ID
	    && m_restrictions.begin()->rt == db::EQ)
	{
	    return RecordsetPtr(new RecordsetOne(m_parent, 
						 m_restrictions.begin()->ival));
	}

	std::string upnp_query = QueryElement(m_root);
	
	TRACE << "Query string is '" << upnp_query << "'\n";
	
	return RecordsetPtr(new SearchRecordset(m_parent, upnp_query));
    }

    TRACE << "Warning, don't know how to do UPnP query (" << ToString() << ")\n";

    return RecordsetPtr(new RecordsetOne(m_parent, 0));
}

} // namespace upnpav
} // namespace db
