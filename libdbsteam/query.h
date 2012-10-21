/* libdbsteam/query.h */
#ifndef DBSTEAM_QUERY_H
#define DBSTEAM_QUERY_H 1

#include "libdb/db.h"
#include "libdb/query_impl.h"
#include <boost/regex.hpp>
#include <list>

namespace db {

namespace steam {

class Database;

class Query: public db::QueryImpl
{
    Database *m_db;

    typedef std::list<boost::regex> regexes_t;
    regexes_t m_regexes;

public:
    explicit Query(Database*);

    bool Match(db::Recordset *rs);

    // Being a db::QueryImpl
    db::RecordsetPtr Execute();
};

}; // namespace steam

}; // namespace db

#endif
