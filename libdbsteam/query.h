/* libdbsteam/query.h */
#ifndef DBSTEAM_QUERY_H
#define DBSTEAM_QUERY_H 1

#include "libdb/query.h"
#include <boost/regex.hpp>
#include <map>

namespace db {

namespace steam {

class Database;

class Query: public db::Query
{
    Database *m_db;

    typedef std::map<unsigned int, boost::regex> regexes_t;
    regexes_t m_regexes;

    bool MatchElement(db::Recordset*, ssize_t);

public:
    explicit Query(Database*);

    bool Match(db::Recordset *rs);

    // Being a db::QueryImpl
    util::CountedPointer<db::Recordset> Execute();
};

typedef util::CountedPointer<db::steam::Query> QueryPtr;

} // namespace steam

} // namespace db

#endif
