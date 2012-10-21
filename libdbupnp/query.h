#ifndef LIBDBUPNP_QUERY_H
#define LIBDBUPNP_QUERY_H 1

#include "libdb/query.h"

namespace db {
namespace upnpav {

class Database;

class Query: public db::Query
{
    Database *m_parent;

    std::string QueryElement(ssize_t);
    
public:
    explicit Query(Database *parent) : m_parent(parent) {}

    // Being a Query
    RecordsetPtr Execute();
    unsigned int CollateBy(field_t which);
};

} // namespace upnpav
} // namespace db

#endif
