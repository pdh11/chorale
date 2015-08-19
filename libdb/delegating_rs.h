#ifndef LIBDB_DELEGATING_RS_H
#define LIBDB_DELEGATING_RS_H

#include "recordset.h"
#include "libutil/counted_pointer.h"

namespace db {

/** A class that does nothing but pass Recordset operations on to another.
 *
 * This is useful to derive from when only overriding one or two operations.
 */
class DelegatingRecordset: public Recordset
{
protected:
    util::CountedPointer<Recordset> m_rs;

public:
    explicit DelegatingRecordset(util::CountedPointer<Recordset> rs)
	: m_rs(rs) {}

    bool IsEOF() const override;
    uint32_t GetInteger(unsigned int which) const override;
    std::string GetString(unsigned int which) const override;
    unsigned int SetInteger(unsigned int which, uint32_t value) override;
    unsigned int SetString(unsigned int which,
                           const std::string& value) override;
    void MoveNext() override;
    unsigned int AddRecord() override;
    unsigned int Commit() override;
    unsigned int Delete() override;
};

} // namespace db

#endif
