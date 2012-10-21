#include "database.h"
#include "config.h"
#include "libmediadb/schema.h"

namespace choraled {

LocalDatabase::LocalDatabase(util::http::Client *client)
    : m_sdb(mediadb::FIELD_COUNT),
      m_ldb(&m_sdb, client)
{
    m_sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::ARTIST,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::ALBUM,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::GENRE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::TITLE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::REMIXED,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::ORIGINALARTIST,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_sdb.SetFieldInfo(mediadb::MOOD,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
}

LocalDatabase::~LocalDatabase()
{
}

unsigned int LocalDatabase::Init(const std::string& loroot,
				 const std::string& hiroot,
				 util::TaskQueue *queue, 
				 const std::string& dbfilename)
{
    return m_ifs.Init(loroot, hiroot, &m_ldb, queue, dbfilename);
}

} // namespace choraled
