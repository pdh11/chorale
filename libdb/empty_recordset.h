#ifndef LIBDB_EMPTY_RECORDSET_H
#define LIBDB_EMPTY_RECORDSET_H 1

#include "readonly_rs.h"

namespace db {

class EmptyRecordset: public ReadOnlyRecordset
{
public:
    bool IsEOF() const { return true; }
    void MoveNext() {}
    uint32_t GetInteger(unsigned int) const { return 0; }
    std::string GetString(unsigned int) const { return std::string(); }
};

} // namespace db

#endif
