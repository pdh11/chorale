#ifndef LIBDBEMPEG_RS_H
#define LIBDBEMPEG_RS_H 1

#include "libdb/delegating_rs.h"

namespace db {

namespace empeg {

class Connection;

/** A Recordset that rewrites the *1 file (and, for playlists, the *0 file too)
 * on Commit(), and deletes the fid on Delete().
 *
 * Everything else is forwarded to the underlying recordset.
 */
class Recordset: public db::DelegatingRecordset
{
    Connection *m_connection;

public:
    Recordset(util::CountedPointer<db::Recordset> rs, Connection *parent)
	: db::DelegatingRecordset(rs), m_connection(parent)
    {
    }

    // Being a Recordset
    unsigned int Commit();
    unsigned int Delete();
};

} // namespace db::empeg

} // namespace db

#endif
