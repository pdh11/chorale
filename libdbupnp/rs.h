#ifndef LIBDBUPNP_RS_H
#define LIBDBUPNP_RS_H 1

#include "libdb/db.h"
#include "libdb/readonly_rs.h"
#include <string>

namespace db {
namespace upnpav {

class Database;

class Recordset: public db::ReadOnlyRecordset
{
protected:
    Database *m_parent;
    db::RecordsetPtr m_freers;
    unsigned int m_id;

    enum {
	GOT_BASIC = 1,
	GOT_TAGS = 2,
	GOT_CHILDREN = 4
    };
    unsigned int m_got_what;

    void GetTags();
    void GetChildren();

public:
    explicit Recordset(Database *parent);

    // Being a Recordset
    uint32_t GetInteger(int which);
    std::string GetString(int which);

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

} // namespace upnpav
} // namespace db

#endif
