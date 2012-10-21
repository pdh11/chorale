#ifndef LIBDBRECEIVER_RS_H
#define LIBDBRECEIVER_RS_H 1

#include "libdb/query.h"
#include "libdb/readonly_rs.h"
#include <list>
#include <string>
#include <vector>

namespace db {
namespace receiver {

class Database;

class Recordset: public db::ReadOnlyRecordset
{
protected:
    Database *m_parent;
    db::RecordsetPtr m_freers;
    unsigned int m_id;

    enum {
	GOT_CONTENT = 1,
	GOT_TAGS = 2,
	GOT_TITLE = 4,
	GOT_TYPE = 8
    };
    unsigned int m_got_what;

    void GetTags();
    void GetContent();

public:
    explicit Recordset(Database *parent);

    // Being a Recordset
    uint32_t GetInteger(field_t which);
    std::string GetString(field_t which);

    // Not MoveNext() or IsEOF() -- implemented in derived classes
};

class RecordsetOne: public Recordset
{
public:
    RecordsetOne(Database *db, unsigned int id);

    // Remaining Recordset methods
    void MoveNext();
    bool IsEOF();
};

class RestrictionRecordset: public Recordset
{
    /** Binary playlist from server
     */
    std::string m_content;
    bool m_eof;
    size_t m_recno;
    
    void LoadID();

public:
    RestrictionRecordset(Database *parent,
			 const Query::Restriction& r);

    // Remaining Recordset methods
    void MoveNext();
    bool IsEOF();
};


class CollateRecordset: public db::ReadOnlyRecordset
{
    Database *m_parent;
    std::vector<std::string> m_values;
    std::vector<unsigned int> m_counts;
    size_t m_recno;
    bool m_eof;

public:
    CollateRecordset(Database *parent,
		     const Query::restrictions_t& restrictions,
		     int collateby);

    // Being a Recordset
    uint32_t GetInteger(field_t which);
    std::string GetString(field_t which);
    void MoveNext();
    bool IsEOF();
};

} // namespace receiver
} // namespace db

#endif
