#ifndef LIBDBRECEIVER_RECORDSET_H
#define LIBDBRECEIVER_RECORDSET_H 1

#include "libdb/query.h"
#include "libdb/readonly_rs.h"
#include "libutil/counted_pointer.h"
#include <list>
#include <string>
#include <vector>

namespace db {
namespace receiver {

class Connection;

class Recordset: public db::ReadOnlyRecordset
{
protected:
    Connection *m_parent;
    unsigned int m_id;

    /** m_freers is populated lazily -- only as GetString/GetInteger
     * ask for things -- but GetString/GetInteger are const, so
     * m_freers must be mutable.
     */
    mutable util::CountedPointer<db::Recordset> m_freers;

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
    explicit Recordset(Connection *parent);

    // Being a Recordset
    uint32_t GetInteger(unsigned int which) const;
    std::string GetString(unsigned int which) const;

    // Not MoveNext() or IsEOF() -- implemented in derived classes
};

class RecordsetOne: public Recordset
{
public:
    RecordsetOne(Connection *db, unsigned int id);

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
    RestrictionRecordset(Connection *parent,
			 const Query::Restriction& r);

    // Remaining Recordset methods
    void MoveNext();
    bool IsEOF() const;
};


class CollateRecordset: public db::ReadOnlyRecordset
{
    Connection *m_parent;
    std::vector<std::string> m_values;
    std::vector<unsigned int> m_counts;
    size_t m_recno;
    bool m_eof;

public:
    CollateRecordset(Connection *parent,
		     const Query::restrictions_t& restrictions,
		     int collateby);

    // Being a Recordset
    uint32_t GetInteger(unsigned int which) const;
    std::string GetString(unsigned int which) const;
    void MoveNext();
    bool IsEOF() const;
};

} // namespace receiver
} // namespace db

#endif
