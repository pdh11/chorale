#ifndef MEDIADB_DB_H
#define MEDIADB_DB_H 1

#include "libdb/db.h"
#include <string>
#include <memory>

namespace util { class Stream; }

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
public:
    virtual unsigned int AllocateID() = 0;
    virtual std::string GetURL(unsigned int id) = 0;
    virtual std::auto_ptr<util::Stream> OpenRead(unsigned int id) = 0;
    virtual std::auto_ptr<util::Stream> OpenWrite(unsigned int id) = 0;
};

} // namespace mediadb

#endif
