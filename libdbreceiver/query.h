#ifndef LIBDBRECEIVER_QUERY_H
#define LIBDBRECEIVER_QUERY_H 1

#include "libdb/db.h"
#include "libdb/query_impl.h"
#include <list>
#include <string>

namespace db {
namespace receiver {

class Database;

class Query: public db::QueryImpl
{
    Database *m_parent;
public:
    explicit Query(Database *parent);

    // Being a QueryImpl
    RecordsetPtr Execute();
};

}; // namespace receiver
}; // namespace db

#endif
