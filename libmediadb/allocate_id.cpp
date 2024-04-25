#include "allocate_id.h"
#include "schema.h"
#include "libdb/query.h"
#include "libdb/db.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libutil/counted_pointer.h"
#include <limits.h>

namespace mediadb {

AllocateID::AllocateID(db::Database *thedb)
    : m_db(thedb),
      m_gap_begin(0x120),
      m_gap_end(0x120)
{

}

unsigned int AllocateID::Allocate()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_gap_begin == m_gap_end)
    {
	db::QueryPtr qp = m_db->CreateQuery();
	qp->CollateBy(mediadb::ID);
	qp->Where(qp->Restrict(mediadb::ID, db::GT, m_gap_end));
	db::RecordsetPtr rs = qp->Execute();
	unsigned int expected = m_gap_end + 1;
	while (!rs->IsEOF())
	{
	    unsigned int found = rs->GetInteger(mediadb::ID);
	    if (found > expected)
	    {
		m_gap_begin = expected;
		m_gap_end = found;
//		TRACE << "Expected " << expected << ", found " << found
//		      << ", " << m_gap_begin << ".." << m_gap_end-1 << " free\n";
		break;
	    }

	    expected = found+1;
	    rs->MoveNext();
	}
	if (rs->IsEOF())
	{
//	    TRACE << "Fell off end, " << expected << "+ all free\n";
	    m_gap_begin = expected;
	    m_gap_end = UINT_MAX;
	}
    }

    return m_gap_begin++;
}

} // namespace mediadb
