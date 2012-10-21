#ifndef LIBDBUPNP_RS_H
#define LIBDBUPNP_RS_H 1

#include "libdb/db.h"
#include "libdb/readonly_rs.h"
#include "libmediadb/didl.h"
#include <string>
#include <vector>

namespace db {
namespace upnpav {

class Database;

class Recordset: public db::ReadOnlyRecordset
{
protected:
    Database *m_parent;
    unsigned int m_id;

    mutable db::RecordsetPtr m_freers;

    enum {
	GOT_BASIC = 1, // Just title and type
	GOT_TAGS = 2,
	GOT_CHILDREN = 4
    };
    mutable unsigned int m_got_what;

    void GetTags() const;
    void GetChildren() const;

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

class CollateRecordset: public db::ReadOnlyRecordset
{
    Database *m_parent;
    field_t m_field;

    enum { LUMP = 32 };

    unsigned int m_start;
    unsigned int m_index;
    unsigned int m_total;
    std::vector<std::string> m_items;

    unsigned int GetSome();

public:
    CollateRecordset(Database *db, field_t field);

    // Being a Recordset
    uint32_t GetInteger(field_t which) const;
    std::string GetString(field_t which) const;
    void MoveNext();
    bool IsEOF() const;
};

class SearchRecordset: public Recordset
{
    std::string m_query;

    enum { LUMP = 32 };

    unsigned int m_start;
    unsigned int m_index;
    unsigned int m_total;

    mediadb::didl::MetadataList m_items;

    unsigned int GetSome();
    void SelectThisItem();

public:
    SearchRecordset(Database *db, const std::string& query);

    // Remaining Recordset methods
    void MoveNext();
    bool IsEOF() const;
};

} // namespace upnpav
} // namespace db

#endif
