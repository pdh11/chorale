#ifndef LIBDB_EMPTY_RECORDSET_H
#define LIBDB_EMPTY_RECORDSET_H 1

#include "readonly_rs.h"

namespace db {

class EmptyRecordset final: public ReadOnlyRecordset
{
public:
    bool IsEOF() const override { return true; }
    void MoveNext() override {}
    uint32_t GetInteger(unsigned int) const override { return 0; }
    std::string GetString(unsigned int) const override { return std::string(); }
};

} // namespace db

#endif
