/* db/free_rs.h
 */
#ifndef DB_FREERS_H
#define DB_FREERS_H

#include <vector>
#include <string>
#include "recordset.h"

namespace db {

/** A freestanding recordset implemented using a vector of strings.
 */
class FreeRecordset: public db::Recordset
{
    std::vector<std::string> m_strings;
    bool m_eof;

    FreeRecordset() : m_eof(false) {}
public:
    static util::CountedPointer<Recordset> Create();

    // Being a Recordset
    bool IsEOF() const;

    uint32_t GetInteger(unsigned int which) const;
    std::string GetString(unsigned int which) const;

    unsigned int SetInteger(unsigned int which, uint32_t value);
    unsigned int SetString(unsigned int which, const std::string& value);

    void MoveNext();
    unsigned int AddRecord();
    unsigned int Commit();
    unsigned int Delete();
};

} // namespace db

#endif
