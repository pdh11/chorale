#ifndef MEDIADB_DB_H
#define MEDIADB_DB_H 1

#include "libdb/db.h"
#include "libutil/stream.h"

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
    virtual std::string GetURL(unsigned int id) = 0;
    virtual util::SeekableStreamPtr OpenRead(unsigned int id) = 0;
    virtual util::SeekableStreamPtr OpenWrite(unsigned int id) = 0;
};

};

#endif
