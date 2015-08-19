#ifndef MEDIADB_LOCALDB_H
#define MEDIADB_LOCALDB_H 1

#include "libmediadb/db.h"
#include "libmediadb/allocate_id.h"
#include "tag_serialiser.h"

namespace util { class SeekableStream; }
namespace util { class TaskQueue; }
namespace util { namespace http { class Client; } }

namespace db {

/** A database representing locally-accessible files in a filesystem
 */
namespace local {

/** A mediadb::Database that wraps up a db::Database and accesses data via
 * its "path" field.
 */
class Database final: public mediadb::Database
{
    db::Database *m_db;
    util::http::Client *m_client;
    util::TaskQueue *m_queue;
    mediadb::AllocateID m_aid;
    TagSerialiser m_tag_serialiser;

    class Recordset;
    class Query;

public:
    /** Construct a db::local::Database.
     *
     * @param thedb  Underlying database (eg a db::steam::Database).
     * @param client An HTTP client, in case of internet radio stations.
     * @param queue  A task queue for re-tagging operations.
     *
     * Passing queue=NULL means tag changes are not reflected to the actual
     * files, and exist only in the database.
     */
    Database(db::Database *thedb, util::http::Client *client,
	     util::TaskQueue *queue = NULL)
	: m_db(thedb),
	  m_client(client), 
	  m_queue(queue), 
	  m_aid(thedb),
	  m_tag_serialiser(this, queue)
    {}

    ~Database();

    db::RecordsetPtr CreateRecordset() override;
    db::QueryPtr CreateQuery() override;

    unsigned int AllocateID() override { return m_aid.Allocate(); }
    std::string GetURL(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenRead(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenWrite(unsigned int id) override;
};

} // namespace db::local
} // namespace db

#endif
