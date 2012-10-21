#include "config.h"
#include "query.h"
#include "db.h"
#include "rs.h"
#include "libutil/trace.h"

namespace db {
namespace steam {

Query::Query(Database *db)
    : m_db(db)
{
}

db::RecordsetPtr Query::Execute()
{
    if (!m_restrictions.empty())
	assert(m_root != 0);

    m_regexes.clear();
    for (restrictions_t::const_iterator i = m_restrictions.begin();
	 i != m_restrictions.end();
	 ++i)
    {
	if (i->rt == db::LIKE)
	    m_regexes[i-m_restrictions.begin()] = boost::regex(i->sval, boost::regex::icase);
    }

    if (!m_collateby.empty())
    {
	unsigned int field = *m_collateby.begin();
	if ((m_db->m_fields[field].flags & FIELD_INDEXED) == 0)
	{
	    TRACE << "Can't collate on unindexed field " << field << "\n";
	    return db::RecordsetPtr();
	}
	if (m_collateby.size() > 1)
	{
	    TRACE << "Can't do multiple collate-by yet\n";
	    return db::RecordsetPtr();
	}
	return db::RecordsetPtr(new CollateRecordset(m_db,
						     *m_collateby.begin(),
						     this));
    }

    if (!m_orderby.empty())
    {
	return db::RecordsetPtr(new OrderedRecordset(m_db,
						     *m_orderby.begin()));
    }

    if (m_restrictions.size() == 1
	&& m_restrictions.begin()->rt == db::EQ)
    {
	int field = m_restrictions.begin()->which;

	if (m_db->m_fields[field].flags & FIELD_INDEXED)
	{
	    if (m_restrictions.begin()->is_string)
		return db::RecordsetPtr(new IndexedRecordset(m_db, field,
							     m_restrictions.begin()->sval));
	    return db::RecordsetPtr(new IndexedRecordset(m_db, field,
							 m_restrictions.begin()->ival));
	}
	// else fall through
    }

    return db::RecordsetPtr(new SimpleRecordset(m_db, this));
}

bool Query::Match(db::Recordset *rs)
{
    if (!m_root)
	return true;
    return MatchElement(rs, m_root);
}

bool Query::MatchElement(db::Recordset *rs, ssize_t elem)
{
    if (elem > 0)
    {
	const Restriction& r = m_restrictions[elem-1];

	if (r.is_string)
	{
	    std::string val = rs->GetString(r.which);
	    switch (r.rt)
	    {
	    case db::EQ:
		if (val != r.sval)
		    return false;
		break;
	    case db::NE:
		if (val == r.sval)
		    return false;
		break;
	    case db::LIKE:
	    {
		bool rc = boost::regex_match(val, m_regexes[elem-1]);
//		TRACE << "'" << val << "' like '" << r.sval << "' : " << rc
//		      << "\n";
		if (!rc)
		    return false;
		break;
	    }
	    default:
		assert(false);
		break;
	    }
	}
	else
	{
	    uint32_t val = rs->GetInteger(r.which);

	    switch (r.rt)
	    {
	    case db::EQ:
		if (val != r.ival)
		    return false;
		break;
	    case db::GT:
		if (val <= r.ival)
		    return false;
		break;
	    default:
		assert(false);
		break;
	    }
	}
    }
    else
    {
	assert(elem < 0);
	
	const Relation& r = m_relations[-elem-1];
	bool lhs = MatchElement(rs, r.a);
	if (r.anditive)
	{
	    if (!lhs)
		return false;
	}
	else
	{
	    if (lhs)
		return true;
	}
	return MatchElement(rs, r.b);
    }

    return true;
}

} // namespace steam
} // namespace db

#ifdef TEST

int main()
{
    db::steam::Test();
    return 0;
}

#endif
