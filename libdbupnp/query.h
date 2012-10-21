#ifndef LIBDBUPNP_QUERY_H
#define LIBDBUPNP_QUERY_H 1

#include "libdb/query.h"

namespace db {
namespace upnp {

class Connection;

class Query: public db::Query
{
    Connection *m_connection;

    std::string QueryElement(ssize_t);
    
public:
    explicit Query(Connection *parent) : m_connection(parent) {}

    // Being a Query
    util::CountedPointer<db::Recordset> Execute();
    unsigned int CollateBy(unsigned int which);
};

} // namespace upnpav
} // namespace db

#endif
