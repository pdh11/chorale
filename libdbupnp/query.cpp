#include "query.h"
#include "rs.h"
#include "libutil/trace.h"
#include "libmediadb/schema.h"

namespace db {
namespace upnpav {

RecordsetPtr Query::Execute()
{
    TRACE << "UPNP query: " << ToString() << "\n";

    if (m_restrictions.size() == 1
	&& m_restrictions.begin()->which == mediadb::ID
	&& m_restrictions.begin()->rt == db::EQ)
    {
	return RecordsetPtr(new RecordsetOne(m_parent, 
					     m_restrictions.begin()->ival));
    }

    TRACE << "Warning, don't know how to do this query\n";

    return RecordsetPtr(new RecordsetOne(m_parent, 0));
}

} // namespace upnpav
} // namespace db
