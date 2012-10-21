#ifndef LIBDBRECEIVER_QUERY_H
#define LIBDBRECEIVER_QUERY_H 1

#include "libdb/query.h"
#include <list>
#include <string>

namespace db {
namespace receiver {

class Connection;

class Query: public db::Query
{
    Connection *m_parent;
public:
    explicit Query(Connection *parent);

    // Being a Query
    util::CountedPointer<db::Recordset> Execute();
};

} // namespace receiver
} // namespace db

#endif
