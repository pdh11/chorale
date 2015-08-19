#include "db.h"
#include "libmediadb/schema.h"
#include "libdb/delegating_rs.h"
#include "libdb/delegating_query.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include "libutil/http_fetcher.h"
#include "libutil/http_stream.h"
#include "libimport/playlist.h"
#include <fcntl.h>
#include <unistd.h>

namespace db {
namespace local {


        /* Recordset */


/** A Recordset that rewrites playlists and tag data on Commit() and deletes
 * the file on Delete().
 *
 * @bug Playlists in Commit!
 */
class Database::Recordset: public db::DelegatingRecordset
{
    Database *m_parent;

public:
    Recordset(db::RecordsetPtr rs, Database *parent)
	: db::DelegatingRecordset(rs), m_parent(parent)
    {
    }

    // Being a Recordset
    unsigned int Commit();
    unsigned int Delete();
};

unsigned int Database::Recordset::Commit()
{
    unsigned int id = m_rs->GetInteger(mediadb::ID);
    unsigned int rc = m_rs->Commit();
    if (rc)
	return rc;

    if (m_parent->m_queue)
	return m_parent->m_tag_serialiser.Rewrite(id);
    else
	TRACE << "Can't edit tags -- no writer queue\n";

    return 0;
}

unsigned int Database::Recordset::Delete()
{
    TRACE << "Chickening out of deleting\n";
    // unlink(m_rs->GetString(mediadb::PATH));

    return m_rs->Delete();
}


        /* Query */


/** A Query that wraps all returned Recordsets in a
 * db::local::Database::Recordset.
 */
class Database::Query: public db::DelegatingQuery
{
    Database *m_parent;

public:
    Query(db::QueryPtr qp, Database *parent)
	: db::DelegatingQuery(qp), m_parent(parent)
    {
    }

    // Being a Query
    db::RecordsetPtr Execute();
};

db::RecordsetPtr Database::Query::Execute()
{
    db::RecordsetPtr rs = m_qp->Execute();
    if (!rs)
	return rs;
    return db::RecordsetPtr(new Recordset(rs, m_parent));
}


        /* Database itself */


Database::~Database()
{
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr(new Recordset(m_db->CreateRecordset(), this));
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(m_db->CreateQuery(), this));
}

std::string Database::GetURL(unsigned int id)
{
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return std::string();
    }

    if (rs->GetInteger(mediadb::TYPE) == mediadb::RADIO)
    {
	// Internet radio station -- URL is inside the "playlist"

	std::string playlist_path = rs->GetString(mediadb::PATH);
	TRACE << "Getting URL for radio station " << playlist_path << "\n";
	import::Playlist p;
	std::list<std::string> entries;
	unsigned int rc = p.Init(playlist_path);
	if (!rc)
	    rc = p.Load(&entries);
	if (rc || entries.empty())
	{
	    TRACE << "Can't load PLS for radio station " << playlist_path
		  << "\n";
	    return std::string();
	}
	return *entries.begin();
    }

    return util::PathToURL(rs->GetString(mediadb::PATH));
}

std::unique_ptr<util::Stream> Database::OpenRead(unsigned int id)
{
    std::unique_ptr<util::Stream> sp;
	
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return sp;
    }

    if (rs->GetInteger(mediadb::TYPE) == mediadb::RADIO)
    {
	// Internet radio station -- URL is inside the "playlist"

	std::string playlist_path = rs->GetString(mediadb::PATH);
	TRACE << "Opening Internet radio station at " << playlist_path << "\n";
	import::Playlist p;
	std::list<std::string> entries;
	unsigned int rc = p.Init(playlist_path);
	if (!rc)
	    rc = p.Load(&entries);
	if (rc || entries.empty())
	{
	    TRACE << "Can't load PLS for radio station " << playlist_path
		  << "\n";
	    return sp;
	}

	for (std::list<std::string>::const_iterator i = entries.begin();
	     i != entries.end();
	     ++i)
	{
	    rc = util::http::Stream::Create(&sp, m_client, i->c_str());
	    if (rc == 0)
		return sp;
	}
	TRACE << "URLs won't open for radio station " << playlist_path << "\n";
	return sp;
    }
    
    unsigned int rc = util::OpenFileStream(rs->GetString(mediadb::PATH).c_str(),
					   util::READ, &sp);
    if (rc)
    {
	TRACE << "Can't open ID " << id << " file " << rs->GetString(mediadb::PATH)
	      << " rc=" << rc << "\n";
    }
    return sp;
}

std::unique_ptr<util::Stream> Database::OpenWrite(unsigned int id)
{
    std::unique_ptr<util::Stream> fsp;

    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return fsp;
    }
    
    unsigned int rc = util::OpenFileStream(rs->GetString(mediadb::PATH).c_str(),
					   util::WRITE, &fsp);
    if (rc)
    {
	TRACE << "Can't open ID " << id << " file " << rs->GetString(mediadb::PATH)
	      << " rc=" << rc << "\n";
    }
    return fsp;
}

} // namespace db::local
} // namespace db

#ifdef TEST

# include "libutil/http_client.h"
# include "libdbsteam/db.h"
# include "libmediadb/xml.h"
# include <stdlib.h>

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
    util::http::Client client;
    mediadb::Database *mdb = new db::local::Database(sdb, &client);

    srand(42);

    std::set<unsigned int> ids;

    for (unsigned int i=0; i<1000; ++i)
    {
	unsigned int id = mdb->AllocateID();
	ids.insert(id);

	db::RecordsetPtr rs = sdb->CreateRecordset();
	rs->AddRecord();
	rs->SetInteger(mediadb::ID, id);
	rs->SetInteger(mediadb::TITLE, i);
	rs->Commit();
//	TRACE << "Adding " << id << "\n";

	if ((i%5) == 4)
	{
	    unsigned int delendum = ChooseRandomID(&ids);
	    db::QueryPtr qp = sdb->CreateQuery();
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
	    mdb = new db::local::Database(sdb, &client);
	    
	    rc = mediadb::ReadXML(sdb, fname);
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
