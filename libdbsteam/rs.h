/* libdbsteam/rs.h
 */
#ifndef DBSTEAM_RS_H
#define DBSTEAM_RS_H 1

#include "libdb/recordset.h"
#include "libdb/readonly_rs.h"
#include "libutil/counted_pointer.h"
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
    explicit Recordset(Database *db);

    bool IsEOF() const;
    uint32_t GetInteger(unsigned int which) const;
    std::string GetString(unsigned int which) const;

    unsigned int SetString(unsigned int which, const std::string&);
    unsigned int SetInteger(unsigned int which, uint32_t);

    unsigned int AddRecord();
    unsigned int Commit();
    unsigned int Delete();

    void MoveNext() = 0;

    void SetRecordNumber(unsigned int recno);
};

class SimpleRecordset: public Recordset
{
    util::CountedPointer<Query> m_query;

public:
    SimpleRecordset(Database*, util::CountedPointer<Query>);
    
    void MoveNext();
};

class CollateRecordset: public ReadOnlyRecordset
{
    Database *m_parent;
    unsigned int m_field;
    bool m_is_int;
    unsigned int m_intvalue;
    std::string m_strvalue;
    bool m_eof;
    util::CountedPointer<Query> m_query;
    SimpleRecordset m_rs;

    void MoveUntilValid(Database::stringindex_t::const_iterator i,
			Database::stringindex_t::const_iterator end);

public:
    CollateRecordset(Database*, unsigned int field,
		     util::CountedPointer<Query>);
    
    bool IsEOF() const;
    uint32_t GetInteger(unsigned int which) const;
    std::string GetString(unsigned int which) const;
    void MoveNext();
};

class IndexedRecordset: public Recordset
{
    unsigned int m_field;
    bool m_is_int;
    std::string m_stringval;
    uint32_t m_intval;
    unsigned int m_subrecno;

public:
    IndexedRecordset(Database *db, unsigned int field, uint32_t intval);
    IndexedRecordset(Database *db, unsigned int field, std::string stringval);
    void MoveNext();
};

class OrderedRecordset: public Recordset
{
    unsigned int m_field;
    bool m_is_int;
    std::string m_stringval;
    uint32_t m_intval;
    unsigned int m_subrecno;

public:
    OrderedRecordset(Database *db, unsigned int field);
    void MoveNext();
};

} // namespace steam

} // namespace db

#endif
