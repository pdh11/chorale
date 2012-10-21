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
    unsigned int m_id;

    /** m_freers is populated lazily -- only as GetString/GetInteger
     * ask for things -- but GetString/GetInteger are const, so
     * m_freers must be mutable.
     */
    mutable db::RecordsetPtr m_freers;

    enum {
	GOT_CONTENT = 1,
	GOT_TAGS = 2,
	GOT_TITLE = 4,
	GOT_TYPE = 8
    };
    mutable unsigned int m_got_what;

    void GetTags() const;
    void GetContent() const;

public:
    explicit Recordset(Database *parent);

    // Being a Recordset
    uint32_t GetInteger(field_t which) const;
    std::string GetString(field_t which) const;

    // Not MoveNext() or IsEOF() -- implemented in derived classes
};

class RecordsetOne: public Recordset
{
public:
    RecordsetOne(Database *db, unsigned int id);

    // Remaining Recordset methods
    void MoveNext();
    bool IsEOF() const;
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
    bool IsEOF() const;
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
    uint32_t GetInteger(field_t which) const;
    std::string GetString(field_t which) const;
    void MoveNext();
    bool IsEOF() const;
};

} // namespace receiver
} // namespace db

#endif
