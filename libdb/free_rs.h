/* db/free_rs.h
 */
#ifndef DB_FREERS_H
#define DB_FREERS_H

#include <vector>
#include <string>
#include "db.h"

namespace db {

/** A freestanding recordset implemented using a vector of strings.
 */
class FreeRecordset: public db::Recordset
{
    std::vector<std::string> m_strings;
    bool m_eof;

    FreeRecordset() : m_eof(false) {}
public:
    static RecordsetPtr Create();

    // Being a Recordset
    bool IsEOF();

    uint32_t GetInteger(int which);
    std::string GetString(int which);

    unsigned int SetInteger(int which, uint32_t value);
    unsigned int SetString(int which, const std::string& value);

    void MoveNext();
    unsigned int AddRecord();
    unsigned int Commit();
    unsigned int Delete();
};

}; // namespace db

#endif
