#include "database.h"
#include "recordset.h"
#include "libutil/locking.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libdb/query.h"
extern "C" {
#include <api0api.h>
}
#include <errno.h>

namespace db {
namespace inno {

class InnoDBUser: public util::PerClassLocking<InnoDBUser>
{
    static unsigned int sm_count;
    bool m_inited;

public:
    InnoDBUser() : m_inited(false) {}

    unsigned Init(const char*);

    ~InnoDBUser();
};

unsigned int InnoDBUser::sm_count = 0;

unsigned int InnoDBUser::Init(const char *filename)
{
    if (m_inited)
	return EALREADY;

    unsigned int rc2 = util::MkdirParents(filename);
    if (!rc2 && !util::DirExists(filename))
	rc2 = util::Mkdir(filename);
    if (rc2)
	return rc2;

    Lock lock(this);
    ++sm_count;
    m_inited = true;

    if (sm_count > 1)
	return 0;

    // First time
    ib_err_t rc = ib_init();
    if (rc != DB_SUCCESS)
    {
	TRACE << "Can't ib_init: " << rc << "\n";
	return EIO;
    }

    ib_cfg_set_defaults();

    ib_cfg_set("data_home_dir", filename);
    ib_cfg_set("log_group_home_dir", filename);
    
    rc = ib_startup(NULL);
    
    if (rc != DB_SUCCESS)
    {
	TRACE << "Can't ib_startup: " << rc << "\n";
	return EIO;
    }

    return 0;
}

InnoDBUser::~InnoDBUser()
{
    Lock lock(this);
    if (m_inited)
    {
	--sm_count;
	if (sm_count == 0)
	    ib_shutdown();
    }
}

    
Database::Database()
    : m_user(NULL)
{
}

Database::~Database()
{
    delete m_user;
}

QueryPtr Database::CreateQuery()
{
    return QueryPtr();
}

RecordsetPtr Database::CreateRecordset()
{
    ib_trx_t txn = ib_trx_begin(IB_TRX_REPEATABLE_READ);
    ib_crsr_t cursor;
    ib_err_t err = ib_cursor_open_table_using_id(m_table_id, txn, &cursor);
    if (err == DB_SUCCESS)
	return RecordsetPtr(new Recordset(txn, cursor));

    TRACE << "Can't create recordset: " << err << "\n";

    return RecordsetPtr();
}

unsigned int Database::Init(const char *filename,
			    const char *tablename,
			    const FieldInfo *fields,
			    unsigned int num_fields)
{
    if (m_user)
	return EALREADY;

    m_user = new InnoDBUser;

    unsigned int rc = m_user->Init(filename);
    if (rc)
	return rc;

    ib_err_t err = ib_table_get_id(tablename, &m_table_id);
    if (err != DB_SUCCESS)
    {
	TRACE << "Can't get table " << tablename << ", trying create\n";

	ib_tbl_sch_t schema;
	err = ib_table_schema_create(tablename, &schema, IB_TBL_COMPACT, 0);
	if (err != DB_SUCCESS)
	{
	    TRACE << "Can't create table schema: " << err << "\n";
	    return EINVAL;
	}

	for (unsigned short i=0; i<num_fields; ++i)
	{
	    char fname[10];
	    sprintf(fname, "f%u", i);
	    if ((fields[i].flags & db::inno::FIELD_TYPEMASK) == db::inno::FIELD_INT)
	    {
		ib_table_schema_add_col(schema, fname, IB_INT,
					IB_COL_UNSIGNED, i, 4);
	    }
	    else
	    {
		ib_table_schema_add_col(schema, fname, IB_VARCHAR,
					IB_COL_NONE, i, 1024);
	    }
	}

	ib_idx_sch_t index;
	ib_table_schema_add_index(schema, "PRIMARY_KEY", &index);
	ib_index_schema_add_col(index, "f0", 0);
	ib_index_schema_set_clustered(index);

	ib_trx_t txn = ib_trx_begin(IB_TRX_SERIALIZABLE);
	ib_schema_lock_exclusive(txn);
	ib_table_create(txn, schema, &m_table_id);
	ib_schema_unlock(txn);
	ib_trx_commit(txn);
	ib_table_schema_delete(schema);
    }

    return 0;
}

} // namespace inno
} // namespace db

#ifdef TEST

# include "libmediadb/schema.h"
# include "libmediadb/xml.h"
# include "libdblocal/db.h"

int main()
{
    db::inno::Database db;

    db::inno::FieldInfo fields[mediadb::FIELD_COUNT] = {
	{ db::inno::FIELD_INT },
	{ db::inno::FIELD_STRING },
	{ db::inno::FIELD_STRING },
	{ db::inno::FIELD_INT }
    };

    unsigned int rc = db.Init("./foo", "foo/bar", fields,
			      sizeof(fields)/sizeof(*fields));
    assert(rc == 0);

    db::local::Database ldb(&db);

    mediadb::ReadXML(&ldb, SRCROOT "/libmediadb/example.xml");

    system("rm -rf ./foo");

    return 0;
}

#endif
