/* dbc.cpp
 *
 * C API to databases
 */
#include "config.h"
#include "dbc.h"
#include "dbc_wrap.h"
#include <map>
#include <string>


        /* db_Recordset */


struct db__Recordset
{
    db::RecordsetPtr rs;
    typedef std::map<int, std::string> stringmap_t;
    stringmap_t m_strings;

    explicit db__Recordset(db::RecordsetPtr r) : rs(r) {}
};

extern "C" int db_Recordset_IsEOF(db_Recordset rs)
{
    return rs->rs->IsEOF();
}

extern "C" uint32_t db_Recordset_GetInteger(db_Recordset rs, int which)
{
    return rs->rs->GetInteger(which);
}

extern "C" const char *db_Recordset_GetString(db_Recordset rs, int which)
{
    db__Recordset::stringmap_t::iterator i = rs->m_strings.find(which);
    if (i == rs->m_strings.end())
    {
	std::string s = rs->rs->GetString(which);
	if (s.empty())
	    return "";
	rs->m_strings[which] = s;
	return rs->m_strings[which].c_str();
    }
    return i->second.c_str();
}

extern "C" void db_Recordset_MoveNext(db_Recordset rs)
{
    rs->m_strings.clear();
    rs->rs->MoveNext();
}

extern "C" void db_Recordset_Free(db_Recordset *prs)
{
    if (*prs)
    {
	delete *prs;
	*prs = NULL;
    }
}


        /* db_Query */


struct db__Query
{
    db::QueryPtr qp;
    explicit db__Query(db::QueryPtr q) : qp(q) {}
};

extern "C" int db_Query_Restrict(db_Query q, int which, db_RestrictionType rt,
				 const char *val)
{
    return q->qp->Restrict(which, (db::RestrictionType)rt, val);
}

extern "C" int db_Query_Restrict2(db_Query q, int which,
				  db_RestrictionType rt, int val)
{
    return q->qp->Restrict(which, (db::RestrictionType)rt, val);
}

extern "C" db_Recordset db_Query_Execute(db_Query q)
{
    db::RecordsetPtr rs = q->qp->Execute();

    return rs ? new db__Recordset(rs) : NULL;
}

extern "C" void db_Query_Free(db_Query *pqp)
{
    if (*pqp)
    {
	delete *pqp;
	*pqp = NULL;
    }
}


        /* db_Database */


struct db__Database
{
    db::DatabasePtr thedb;

    explicit db__Database(db::DatabasePtr d) : thedb(d) {}
};

db_Database db_Database_Wrap(db::DatabasePtr db)
{
    return new db__Database(db);
}

extern "C" db_Recordset db_Database_CreateRecordset(db_Database d)
{
    db::RecordsetPtr rs = d->thedb->CreateRecordset();

    return rs ? new db__Recordset(rs) : NULL;
}

extern "C" db_Query db_Database_CreateQuery(db_Database d)
{
    db::QueryPtr qp = d->thedb->CreateQuery();

    return qp ? new db__Query(qp) : NULL;
}

extern "C" void db_Database_Free(db_Database *pdb)
{
    if (*pdb)
    {
	delete *pdb;
	*pdb = NULL;
    }
}
