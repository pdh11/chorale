#include "recordset.h"
#include "libutil/trace.h"

namespace db {
namespace inno {

Recordset::Recordset(ib_trx_t txn, ib_crsr_t cursor)
    : m_txn(txn), m_cursor(cursor), m_state(READ), m_eof(false)
{
    m_tuple = ib_clust_read_tuple_create(m_cursor);
    m_former_tuple = ib_clust_read_tuple_create(m_cursor);

    ib_err_t err = ib_cursor_first(m_cursor);
    if (err != DB_SUCCESS)
    {
	TRACE << "Can't find any records in cursor: " << err << "\n";
	m_eof = true;
    }
    else
    {
	err = ib_cursor_read_row(m_cursor, m_tuple);
	if (err != DB_SUCCESS)
	{
	    TRACE << "Can't read row in cursor: " << err << "\n";
	    m_eof = true;
	}
    }
}

Recordset::~Recordset()
{
    ib_tuple_clear(m_tuple);
    ib_tuple_clear(m_former_tuple);
    ib_tuple_delete(m_tuple);
    ib_tuple_delete(m_former_tuple);
    ib_cursor_close(m_cursor);
    ib_trx_commit(m_txn);
}

// Being a Recordset
bool Recordset::IsEOF()
{
    return m_eof;
}

uint32_t Recordset::GetInteger(unsigned int which)
{
    ib_u32_t value = 0;
    ib_tuple_read_u32(m_tuple, which, &value);
    return value;
}

std::string Recordset::GetString(unsigned int which)
{
    size_t len = ib_col_get_len(m_tuple, which);
    if (len > 1024)
	return std::string();

    const char *ptr = (const char*)ib_col_get_value(m_tuple, which);
    return std::string(ptr, ptr+len);
}

unsigned int Recordset::SetInteger(unsigned int which, uint32_t value)
{
    ib_col_meta_t column_metadata;
    ib_col_get_meta(m_tuple, which, &column_metadata);

    if (column_metadata.type == IB_VARCHAR)
    {
	char str[20];
	sprintf(str, "%u", value);
	return SetString(which, str);
    }

    if (m_state == READ)
    {
	ib_tuple_copy(m_former_tuple, m_tuple);
	m_state = UPDATE;
    }

    ib_err_t err = ib_tuple_write_u32(m_tuple, which, value);
    if (err != DB_SUCCESS)
	return EINVAL;

    return 0;
}

unsigned int Recordset::SetString(unsigned int which,
				  const std::string& value)
{
    ib_col_meta_t column_metadata;
    ib_col_get_meta(m_tuple, which, &column_metadata);

    if (column_metadata.type != IB_VARCHAR)
    {
	return SetInteger(which, atoi(value.c_str()));
    }

    if (m_state == READ)
    {
	ib_tuple_copy(m_former_tuple, m_tuple);
	m_state = UPDATE;
    }

    ib_err_t err = ib_col_set_value(m_tuple, which, value.c_str(),
				    value.length());
    if (err != DB_SUCCESS)
	return EINVAL;

    return 0;
}

void Recordset::MoveNext()
{
    m_state = READ;

    if (m_eof)
	return;

    ib_tuple_clear(m_former_tuple);
    ib_tuple_clear(m_tuple);
    ib_err_t err = ib_cursor_next(m_cursor);
    if (err != DB_SUCCESS)
    {
	TRACE << "Can't movenext: " << err << "\n";
	m_eof = true;
	return;
    }

    err = ib_cursor_read_row(m_cursor, m_tuple);
    if (err != DB_SUCCESS)
    {
	TRACE << "Can't read next row: " << err << "\n";
	m_eof = true;
	return;
    }
}

unsigned int Recordset::AddRecord()
{
    ib_tuple_clear(m_tuple);
    ib_tuple_clear(m_former_tuple);
    m_tuple = ib_clust_read_tuple_create(m_cursor);
    m_state = INSERT;
    return 0;
}

unsigned int Recordset::Commit()
{
    ib_err_t err;

    switch (m_state)
    {
    case INSERT:
	err = ib_cursor_insert_row(m_cursor, m_tuple);
	break;
    case UPDATE:
	err = ib_cursor_update_row(m_cursor, m_former_tuple, m_tuple);
	break;
    default:
	return 0;
    }

    if (err != DB_SUCCESS)
    {
	TRACE << "Can't pre-commit: " << err << "\n";
	return EINVAL;
    }
    
    m_state = READ;

    ib_cursor_reset(m_cursor);
    err = ib_trx_commit(m_txn);
    if (err != DB_SUCCESS)
    {
	TRACE << "Can't commit: " << err << "\n";
	return EINVAL;
    }

    ib_tuple_clear(m_former_tuple); // m_tuple is still live though

    m_txn = ib_trx_begin(IB_TRX_SERIALIZABLE);
    ib_cursor_attach_trx(m_cursor, m_txn);

    TRACE << "Committed OK\n";

    return 0;
}

unsigned int Recordset::Delete()
{
    return EPERM;
}

} // namespace inno
} // namespace db
