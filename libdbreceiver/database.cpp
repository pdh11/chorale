#include "database.h"
#include "query.h"
#include "libdb/recordset.h"
#include "libutil/stream.h"
#include "libutil/counted_pointer.h"

namespace db {
namespace receiver {

Database::Database(util::http::Client *http)
    : m_connection(http)
{
}

Database::~Database()
{
}

unsigned int Database::Init(const util::IPEndPoint& ep)
{
    return m_connection.Init(ep);
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr();
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(&m_connection));
}

std::string Database::GetURL(unsigned int id)
{
    return m_connection.GetURL(id);
}

std::unique_ptr<util::Stream> Database::OpenRead(unsigned int id)
{
    return m_connection.OpenRead(id);
}

std::unique_ptr<util::Stream> Database::OpenWrite(unsigned int)
{
    return std::unique_ptr<util::Stream>();
}

} // namespace receiver
} // namespace db
