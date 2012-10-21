#include "config.h"
#include "db.h"
#include "query.h"
#include "rs.h"
#include "libutil/trace.h"

namespace db {
namespace steam {

Database::Database(field_t nfields, const InitialFieldInfo *ifi)
    : m_nfields(nfields),
      m_next_recno(0)
{
    m_fields.resize(nfields);
    m_stringindexes.resize(nfields);
    m_intindexes.resize(nfields);

    if (ifi)
    {
	while (ifi->which || ifi->flags)
	{
	    TRACE << "field " << ifi->which << ": " << ifi->flags << "\n";
	    m_fields[ifi->which].flags = ifi->flags;
	    ++ifi;
	}
    }
}

Database::~Database()
{
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr(new SimpleRecordset(this, QueryPtr()));
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(this));
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
