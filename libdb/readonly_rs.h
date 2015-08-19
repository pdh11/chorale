/* db/readonly_rs.h
 */
#ifndef DB_READONLY_RS_H
#define DB_READONLY_RS_H

#include "libdb/recordset.h"

namespace db {

/** A helper class for read-only Recordset implementations.
 *
 * Returns EPERM from all calls which would modify the recordset.
 */
class ReadOnlyRecordset: public Recordset
{
public:    
    unsigned int SetInteger(unsigned int, uint32_t) override;
    unsigned int SetString(unsigned int, const std::string&) override;
    unsigned int AddRecord() override;
    unsigned int Commit() override;
    unsigned int Delete() override;
};

} // namespace db

#endif
