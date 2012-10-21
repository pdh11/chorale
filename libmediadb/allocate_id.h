#ifndef MEDIADB_ALLOCATE_ID_H
#define MEDIADB_ALLOCATE_ID_H 1

#include <boost/thread/mutex.hpp>

namespace db { class Database; }

namespace mediadb {

class AllocateID
{
    db::Database *m_db;
    boost::mutex m_mutex;
    unsigned int m_gap_begin;
    unsigned int m_gap_end;

public:
    explicit AllocateID(db::Database*);

    unsigned int Allocate();
};

} // namespace mediadb

#endif
