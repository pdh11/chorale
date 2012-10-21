#ifndef MEDIADB_DB_H
#define MEDIADB_DB_H 1

#include "libdb/db.h"
#include "allocate_id.h"
#include <string>

namespace util { class SeekableStream; }
namespace util { template<class> class CountedPointer; }

/** Classes specialising the general-purpose db::Database API for
 * media databases.
 */
namespace mediadb
{

/** A mediadb::Database extends a Database with methods for getting to the
 * actual tune data.
 */
class Database: public db::Database
{
    class AllocateID m_aid;

public:
    Database() : m_aid(this) {}

    virtual unsigned int AllocateID() { return m_aid.Allocate(); }
    virtual std::string GetURL(unsigned int id) = 0;
    virtual util::CountedPointer<util::SeekableStream> OpenRead(unsigned int id) = 0;
    virtual util::CountedPointer<util::SeekableStream> OpenWrite(unsigned int id) = 0;
};

} // namespace mediadb

#endif
