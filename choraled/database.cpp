#include "database.h"
#include "config.h"
#include "libmediadb/schema.h"
#include "libdblocal/database_updater.h"

namespace choraled {

static const db::steam::Database::InitialFieldInfo field_info[] = 
{
    { mediadb::ID,      db::steam::FIELD_INT   |db::steam::FIELD_INDEXED },
    { mediadb::PATH,    db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { mediadb::ARTIST,  db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { mediadb::ALBUM,   db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { mediadb::GENRE,   db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { mediadb::TITLE,   db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { mediadb::REMIXED, db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { mediadb::ORIGINALARTIST, db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { mediadb::MOOD,    db::steam::FIELD_STRING|db::steam::FIELD_INDEXED },
    { 0,0 }
};

LocalDatabase::LocalDatabase(util::http::Client *client)
    : m_sdb(mediadb::FIELD_COUNT, field_info),
      m_ldb(&m_sdb, client),
      m_database_updater(NULL)
{
}

LocalDatabase::~LocalDatabase()
{
    delete m_database_updater;
}

unsigned int LocalDatabase::Init(const std::string& loroot,
				 const std::string& hiroot,
				 util::Scheduler *scheduler,
				 util::TaskQueue *queue, 
				 const std::string& dbfilename)
{
    assert(!m_database_updater);
    m_database_updater = new db::local::DatabaseUpdater(loroot, hiroot, 
							&m_sdb, &m_ldb,
							scheduler, queue,
							dbfilename);
    return 0;
}

void LocalDatabase::ForceRescan()
{
    if (m_database_updater)
	m_database_updater->ForceRescan(); 
}

} // namespace choraled
