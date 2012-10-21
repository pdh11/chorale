/* db/readonly_rs.h
 */
#ifndef DB_READONLY_RS_H
#define DB_READONLY_RS_H

#include "libdb/db.h"

namespace db {

/** A helper class for read-only Recordset implementations.
 *
 * Returns EPERM from all calls which would modify the recordset.
 */
class ReadOnlyRecordset: public Recordset
{
public:    
    unsigned int SetInteger(int, uint32_t);
    unsigned int SetString(int, const std::string&);
    unsigned int AddRecord();
    unsigned int Commit();
    unsigned int Delete();
};

} // namespace db

#endif
