#ifndef LIBDBEMPEG_QUERY_H
#define LIBDBEMPEG_QUERY_H 1

#include "libdb/delegating_query.h"

namespace db {

namespace empeg {

class Connection;

/** A Query that wraps all returned Recordsets in a db::empeg::Recordset
 */
class Query: public db::DelegatingQuery
{
    Connection *m_parent;

public:
    explicit Query(util::CountedPointer<db::Query> qp, Connection *parent)
	: db::DelegatingQuery(qp), m_parent(parent)
    {
    }

    // Being a Query
    util::CountedPointer<db::Recordset> Execute();
};

} // namespace db::empeg

} // namespace db

#endif
