#include "localdb.h"
#include "schema.h"
#include "libdb/delegating_rs.h"
#include "libdb/delegating_query.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include <fcntl.h>

namespace mediadb {


        /* Recordset */


/** A Recordset that rewrites playlists and tag data on Commit() and deletes
 * the file on Delete().
 */
class LocalDatabase::Recordset: public db::DelegatingRecordset
{
    LocalDatabase *m_parent;

public:
    Recordset(db::RecordsetPtr rs, LocalDatabase *parent)
	: db::DelegatingRecordset(rs), m_parent(parent)
    {
    }

    // Being a Recordset
    unsigned int Commit();
    unsigned int Delete();
};

unsigned int LocalDatabase::Recordset::Commit()
{
//    TRACE << "Chickening out of committing\n";
    return m_rs->Commit();
}

unsigned int LocalDatabase::Recordset::Delete()
{
//    TRACE << "Chickening out of deleting\n";
    // unlink(m_rs->GetString(mediadb::PATH));

    return m_rs->Delete();
}


        /* Query */


/** A Query that wraps all returned Recordsets in a
 * mediadb::LocalDatabase::Recordset.
 */
class LocalDatabase::Query: public db::DelegatingQuery
{
    LocalDatabase *m_parent;

public:
    Query(db::QueryPtr qp, LocalDatabase *parent)
	: db::DelegatingQuery(qp), m_parent(parent)
    {
    }

    // Being a Query
    db::RecordsetPtr Execute();
};

db::RecordsetPtr LocalDatabase::Query::Execute()
{
    db::RecordsetPtr rs = m_qp->Execute();
    if (!rs)
	return rs;
    return db::RecordsetPtr(new Recordset(rs, m_parent));
}

        /* LocalDatabase itself */


db::RecordsetPtr LocalDatabase::CreateRecordset()
{
    return db::RecordsetPtr(new Recordset(m_db->CreateRecordset(), this));
}

db::QueryPtr LocalDatabase::CreateQuery()
{
    return db::QueryPtr(new Query(m_db->CreateQuery(), this));
}

std::string LocalDatabase::GetURL(unsigned int id)
{
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return std::string();
    }

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    factories_t::const_iterator i = m_factories.find(type);
    if (i != m_factories.end())
	return i->second->GetURL(rs);

    return util::PathToURL(rs->GetString(PATH));
}

util::SeekableStreamPtr LocalDatabase::OpenRead(unsigned int id)
{
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return util::SeekableStreamPtr();
    }

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    factories_t::const_iterator i = m_factories.find(type);
    if (i != m_factories.end())
	return i->second->OpenRead(rs);
    
    util::SeekableStreamPtr fsp;
    unsigned int rc = util::OpenFileStream(rs->GetString(PATH).c_str(),
					   util::READ, &fsp);
    if (rc)
    {
	TRACE << "Can't open ID " << id << " file " << rs->GetString(PATH)
	      << " rc=" << rc << "\n";
	return util::SeekableStreamPtr();
    }
    return fsp;
}

util::SeekableStreamPtr LocalDatabase::OpenWrite(unsigned int id)
{
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return util::SeekableStreamPtr();
    }

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    factories_t::const_iterator i = m_factories.find(type);
    if (i != m_factories.end())
	return i->second->OpenWrite(rs);
    
    util::SeekableStreamPtr fsp;
    unsigned int rc = util::OpenFileStream(rs->GetString(PATH).c_str(),
					   util::WRITE, &fsp);
    if (rc)
    {
	TRACE << "Can't open ID " << id << " file " << rs->GetString(PATH)
	      << " rc=" << rc << "\n";
	return util::SeekableStreamPtr();
    }
    return fsp;
}

} // namespace mediadb

#ifdef TEST

# include "libdbsteam/db.h"
# include "xml.h"

typedef std::set<unsigned int> set_t;

static unsigned int ChooseRandomID(const set_t *ids)
{
    unsigned int count = 1;

    set_t::const_iterator i = ids->begin();
    unsigned int id = *i;

    while (++i != ids->end())
    {
	++count;
	if ((rand() % count) == 0)
	    id = *i;
    }

    return id;
}

static void CheckDB(const set_t *ids, db::Database *db)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->OrderBy(mediadb::ID);
    db::RecordsetPtr rs = qp->Execute();

    set_t::const_iterator i = ids->begin();

    while (i != ids->end() && !rs->IsEOF())
    {
//	TRACE << *i << " " << rs->GetInteger(mediadb::ID) << "\n";

	assert(*i == rs->GetInteger(mediadb::ID));
	++i;
	rs->MoveNext();
    }

    assert(i == ids->end());
    assert(rs->IsEOF());
}

int main()
{
    db::steam::Database *sdb = new db::steam::Database(mediadb::FIELD_COUNT);
    sdb->SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    mediadb::LocalDatabase *mdb = new mediadb::LocalDatabase(sdb);

    srand(42);

    std::set<unsigned int> ids;

    for (unsigned int i=0; i<1000; ++i)
    {
	unsigned int id = mdb->AllocateID();
	ids.insert(id);

	db::RecordsetPtr rs = mdb->CreateRecordset();
	rs->AddRecord();
	rs->SetInteger(mediadb::ID, id);
	rs->SetInteger(mediadb::TITLE, i);
	rs->Commit();
//	TRACE << "Adding " << id << "\n";

	if ((i%5) == 4)
	{
	    unsigned int delendum = ChooseRandomID(&ids);
	    db::QueryPtr qp = mdb->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, delendum));
	    rs = qp->Execute();
	    if (!rs || rs->IsEOF())
	    {
		TRACE << "Can't find id " << delendum << "\n";
		assert(false);
	    }
//	    TRACE << "Deleting " << delendum << "\n";
	    rs->Delete();
	    ids.erase(delendum);
	}

#ifndef WIN32
	if ((i%17) == 16)
	{
//	    TRACE << "Bouncing\n";

	    char fname[20] = "test.xml.XXXXXX";
	    int fd = mkstemp(fname);

	    FILE *f = fdopen(fd, "w+");
	    assert(f != NULL);
	    unsigned int rc = mediadb::WriteXML(mdb, 1, f);
	    fclose(f); // closes the fd too
	    assert(rc == 0);

	    delete mdb;
	    delete sdb;
	    sdb = new db::steam::Database(mediadb::FIELD_COUNT);
	    sdb->SetFieldInfo(mediadb::ID, 
			      db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
	    mdb = new mediadb::LocalDatabase(sdb);
	    
	    rc = mediadb::ReadXML(mdb, fname);
	    assert(rc == 0);

	    unlink(fname);
	}
#endif
	
//	TRACE << "Checking " << i << "\n";
	CheckDB(&ids, mdb);
    }

    delete mdb;
    delete sdb;

    return 0;
}

#endif
