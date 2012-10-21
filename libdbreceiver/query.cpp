#include "query.h"
#include "db.h"
#include "rs.h"
#include "libmediadb/schema.h"
#include "libutil/trace.h"

namespace db {
namespace receiver {

Query::Query(Database *parent)
    : m_parent(parent)
{
}

RecordsetPtr Query::Execute()
{
    if (m_restrictions.size() == 1
	&& m_restrictions.begin()->which == mediadb::ID
	&& m_restrictions.begin()->rt == db::EQ)
    {
	return RecordsetPtr(new RecordsetOne(m_parent, 
					     m_restrictions.begin()->ival));
    }

    if (!m_collateby.empty())
    {
	/* Some care needed here, as legacy servers (ARMGR, Central,
	 * receiverd) can only collate by one field at a time. Also, they
	 * can't mix restrictions and collate-by.
	 */
	return RecordsetPtr(new CollateRecordset(m_parent,
						 m_restrictions,
						 *m_collateby.begin()));
    }

    if (m_restrictions.size() == 1)
    {
	return RecordsetPtr(new RestrictionRecordset(m_parent,
						     *m_restrictions.begin()));
    }

    TRACE << "Warning, don't know how to do query \"" << ToString() << "\"\n";

    return RecordsetPtr();
}

} // namespace receiver
} // namespace db
