#ifndef LIBDBEMPEG_ALLOCATE_ID
#define LIBDBEMPEG_ALLOCATE_ID 1

#include "libutil/mutex.h"

namespace db {

class Database;

namespace empeg {

/** Very like mediadb::AllocateID, but knows that Empeg FIDs are multiples of
 * sixteen.
 */
class AllocateID
{
    db::Database *m_db;
    util::Mutex m_mutex;
    unsigned int m_gap_begin;
    unsigned int m_gap_end;

public:
    explicit AllocateID(db::Database*);

    unsigned int Allocate();
};

} // namespace db::empeg

} // namespace db

#endif

