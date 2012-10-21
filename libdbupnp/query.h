#ifndef LIBDBUPNP_QUERY_H
#define LIBDBUPNP_QUERY_H 1

#include "libdb/db.h"

namespace db {
namespace upnpav {

class Database;

class Query: public db::Query
{
    Database *m_parent;
    
public:
    explicit Query(Database *parent) : m_parent(parent) {}

    // Being a QueryImpl
    RecordsetPtr Execute();
};

} // namespace upnpav
} // namespace db

#endif
