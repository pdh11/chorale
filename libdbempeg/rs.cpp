#include "rs.h"
#include "connection.h"

namespace db {

namespace empeg {

unsigned int Recordset::Commit()
{
    unsigned int rc = m_rs->Commit();
    if (rc)
	return rc;

    return m_connection->RewriteData(m_rs);
}

unsigned int Recordset::Delete()
{
    unsigned int rc = m_connection->Delete(m_rs);
    if (rc)
	return rc;

    return m_rs->Delete();
}

} // namespace db::empeg

} // namespace db
