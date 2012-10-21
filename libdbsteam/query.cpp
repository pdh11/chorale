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
    m_regexes.clear();
    for (restrictions_t::const_iterator i = m_restrictions.begin();
	 i != m_restrictions.end();
	 ++i)
    {
	if (i->rt == db::LIKE)
	    m_regexes.push_back(boost::regex(i->sval, boost::regex::icase));
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
    regexes_t::iterator j = m_regexes.begin();

    for (restrictions_t::const_iterator i = m_restrictions.begin();
	 i != m_restrictions.end();
	 ++i)
    {
	if (i->is_string)
	{
	    std::string val = rs->GetString(i->which);
	    switch (i->rt)
	    {
	    case db::EQ:
		if (val != i->sval)
		    return false;
		break;
	    case db::LIKE:
	    {
		bool rc = boost::regex_match(val, *j);
//		TRACE << "'" << val << "' like '" << i->sval << "' : " << rc
//		      << "\n";
		if (!rc)
		    return false;
		++j;
		break;
	    }
	    default:
		assert(false);
		break;
	    }
	}
	else
	{
	    uint32_t val = rs->GetInteger(i->which);

	    switch (i->rt)
	    {
	    case db::EQ:
		if (val != i->ival)
		    return false;
		break;
	    case db::GT:
		if (val <= i->ival)
		    return false;
		break;
	    default:
		assert(false);
		break;
	    }
	}
    }

    return true;
}

}; // namespace steam
}; // namespace db

#ifdef TEST

int main()
{
    db::steam::Test();
    return 0;
}

#endif
