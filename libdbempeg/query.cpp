#include "query.h"
#include "rs.h"
#include "libdb/db.h"

namespace db {

namespace empeg {

RecordsetPtr Query::Execute()
{
    db::RecordsetPtr rs = m_qp->Execute();
    if (!rs)
	return rs;
    return db::RecordsetPtr(new db::empeg::Recordset(rs, m_parent));
}

} // namespace db::empeg

} // namespace db
