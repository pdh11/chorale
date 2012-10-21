#ifndef CHORALED_DATABASE_H
#define CHORALED_DATABASE_H 1

#include "config.h"
#include "libdbsteam/db.h"
#include "libdblocal/db.h"

#define HAVE_LOCAL_DB HAVE_TAGLIB

namespace util { class Scheduler; }
namespace util { class TaskQueue; }
namespace util { namespace http { class Client; } }
namespace db { namespace local { class DatabaseUpdater; } }

namespace choraled {

class LocalDatabase
{
    db::steam::Database m_sdb;
    db::local::Database m_ldb;
    db::local::DatabaseUpdater *m_database_updater;

public:
    LocalDatabase(util::http::Client *client);
    ~LocalDatabase();

    unsigned int Init(const std::string& loroot,
		      const std::string& hiroot,
		      util::Scheduler *scheduler,
		      util::TaskQueue *queue, 
		      const std::string& dbfilename);
    
    db::local::Database *Get() { return &m_ldb; }

    void ForceRescan();
};

} // namespace choraled

#endif
