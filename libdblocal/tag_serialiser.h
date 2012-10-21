#ifndef LIBDB_LOCAL_TAG_SERIALISER_H
#define LIBDB_LOCAL_TAG_SERIALISER_H 1

#include <set>
#include "libutil/mutex.h"

namespace util { class TaskQueue; }
namespace mediadb { class Database; }

namespace db {
namespace local {

/** A serialiser for tagging operations.
 *
 * Every time someone calls Commit on a db::local::Database recordset,
 * the tag changes need to be reflected in the ID3 or similar tags of
 * the underlying file. That's a potentially quite time-consuming
 * operation, so it's punted to a background TaskQueue. But it can't
 * be punted blindly, or two calls to Commit in quick succession on
 * the same ID will tangle each other up. So we keep track of which
 * IDs we currently have outstanding tagging operations on, and, if a
 * new request comes in, we try and assign it to an existing
 * tagging-task for that ID before creating a new task.
 */
class TagSerialiser
{
    mediadb::Database *m_database;
    util::TaskQueue *m_queue;

    util::Mutex m_mutex;
    std::set<unsigned int> m_need_rewrite;
    std::set<unsigned int> m_rewrite_tasks;

    class Task;
    
public:
    TagSerialiser(mediadb::Database *database, util::TaskQueue *queue);

    unsigned int Rewrite(unsigned int id);
};

} // namespace local
} // namespace db

#endif
