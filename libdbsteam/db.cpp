#include "config.h"
#include "db.h"
#include "query.h"
#include "rs.h"

namespace db {
namespace steam {

Database::Database(int nfields)
    : m_nfields(nfields),
      m_next_recno(0)
{
    m_fields.resize(nfields);
    m_stringindexes.resize(nfields);
    m_intindexes.resize(nfields);
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr(new SimpleRecordset(this));
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(this));
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
