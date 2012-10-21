#ifndef LIBDB_DELEGATING_RS_H
#define LIBDB_DELEGATING_RS_H

#include "db.h"

namespace db {

/** A class that does nothing but pass Recordset operations on to another.
 *
 * This is useful to derive from when only overriding one or two operations.
 */
class DelegatingRecordset: public Recordset
{
protected:
    db::RecordsetPtr m_rs;

public:
    explicit DelegatingRecordset(db::RecordsetPtr rs) : m_rs(rs) {}

    bool IsEOF();
    uint32_t GetInteger(field_t which);
    std::string GetString(field_t which);
    unsigned int SetInteger(field_t which, uint32_t value);
    unsigned int SetString(field_t which, const std::string& value);
    void MoveNext();
    unsigned int AddRecord();
    unsigned int Commit();
    unsigned int Delete();
};

} // namespace db

#endif
