/* libdbsteam/rs.h
 */
#ifndef DBSTEAM_RS_H
#define DBSTEAM_RS_H 1

#include "libdb/db.h"
#include "libdb/readonly_rs.h"
#include "db.h"
#include <string>

namespace db {

namespace steam {

class Database;
class Query;

class Recordset: public db::Recordset
{
protected:
    Database *m_db;
    unsigned int m_record;
    bool m_eof;

public:
    Recordset(Database *db);

    bool IsEOF();
    uint32_t GetInteger(int which);
    std::string GetString(int which);

    unsigned int SetString(int which, const std::string&);
    unsigned int SetInteger(int which, uint32_t);

    unsigned int AddRecord();
    unsigned int Commit();
    unsigned int Delete();

    void MoveNext() = 0;

    void SetRecordNumber(unsigned int recno);
};

class SimpleRecordset: public Recordset
{
    Query *m_query;

public:
    SimpleRecordset(Database*, Query* = NULL);
    
    void MoveNext();
};

class CollateRecordset: public ReadOnlyRecordset
{
    Database *m_parent;
    int m_field;
    bool m_is_int;
    unsigned int m_intvalue;
    std::string m_strvalue;
    bool m_eof;
    Query *m_query;
    SimpleRecordset m_rs;

    void MoveUntilValid(Database::stringindex_t::const_iterator i,
			Database::stringindex_t::const_iterator end);

public:
    CollateRecordset(Database*, int field, Query* = NULL);
    
    bool IsEOF();
    uint32_t GetInteger(int which);
    std::string GetString(int which);
    void MoveNext();
};

class IndexedRecordset: public Recordset
{
    int m_field;
    bool m_is_int;
    std::string m_stringval;
    uint32_t m_intval;
    unsigned int m_subrecno;

public:
    IndexedRecordset(Database *db, int field, uint32_t intval);
    IndexedRecordset(Database *db, int field, std::string stringval);
    void MoveNext();
};

class OrderedRecordset: public Recordset
{
    int m_field;
    bool m_is_int;
    std::string m_stringval;
    uint32_t m_intval;
    unsigned int m_subrecno;

public:
    OrderedRecordset(Database *db, int field);
    void MoveNext();
};

} // namespace steam

} // namespace db

#endif
