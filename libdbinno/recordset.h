#ifndef LIBDBINNO_RECORDSET_H
#define LIBDBINNO_RECORDSET_H 1

#include "libdb/recordset.h"
extern "C" {
#include <api0api.h>
}

namespace db {
namespace inno {

class Recordset: public db::Recordset
{
    ib_trx_t m_txn;
    ib_crsr_t m_cursor;
    ib_tpl_t m_former_tuple;
    ib_tpl_t m_tuple;
    enum state_t { READ, INSERT, UPDATE } m_state;
    bool m_eof;

public:
    /** Passes over ownership of the transaction and the cursor
     */
    Recordset(ib_trx_t, ib_crsr_t);
    ~Recordset();

    // Being a Recordset
    bool IsEOF();

    uint32_t GetInteger(unsigned int which);
    std::string GetString(unsigned int which);

    unsigned int SetInteger(unsigned int which, uint32_t value);
    unsigned int SetString(unsigned int which,
			   const std::string& value);

    void MoveNext();
    unsigned int AddRecord();
    unsigned int Commit();

    /** Deletes current record, if any. Implicitly does MoveNext.
     */
    unsigned int Delete();
};

} // namespace inno
} // namespace db

#endif
