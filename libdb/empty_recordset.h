#ifndef LIBDB_EMPTY_RECORDSET_H
#define LIBDB_EMPTY_RECORDSET_H 1

#include "readonly_rs.h"

namespace db {

class EmptyRecordset: public ReadOnlyRecordset
{
public:
    bool IsEOF() { return true; }
    void MoveNext() {}
    uint32_t GetInteger(field_t) { return 0; }
    std::string GetString(field_t) { return std::string(); }
};

} // namespace db

#endif
