#include "db.h"
#include "libmediadb/schema.h"
#include "libdb/empty_recordset.h"
#include "libdb/delegating_rs.h"
#include "libdb/query.h"
#include "libutil/locking.h"
#include "libutil/trace.h"
#include "libutil/stream.h"
#include "libutil/errors.h"
#include <vector>
#include <algorithm>

namespace db {
namespace merge {

class Database::Impl final: public mediadb::Database,
                            public util::PerObjectLocking
{
    std::vector<mediadb::Database*> m_databases;
    
    friend class Query;
    friend class RootRecordset;

public:
    Impl();
    ~Impl();

    unsigned int AddDatabase(mediadb::Database*);
    unsigned int RemoveDatabase(mediadb::Database*);

    // Being a db::Database
    RecordsetPtr CreateRecordset() override;
    QueryPtr CreateQuery() override;

    // Being a mediadb::Database
    unsigned int AllocateID() override;
    std::string GetURL(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenRead(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenWrite(unsigned int id) override;
};

/** A query that returns merged results from all the available databases.
 */
class Database::Query final: public db::Query
{
    Database::Impl *m_database;

public:
    explicit Query(Database::Impl *database)
	: m_database(database)
    {}
    
    // Being a Query
    db::RecordsetPtr Execute() override;
};

/** A recordset that adds the top-eight-bits database identifier into all IDs.
 * 
 * And strips them out again on the way back in.
 */
class Database::WrapRecordset final: public db::DelegatingRecordset
{
    unsigned int m_id;

public:
    WrapRecordset(db::RecordsetPtr rs, unsigned int id)
	: db::DelegatingRecordset(rs),
	  m_id(id)
    {}

    uint32_t GetInteger(unsigned int which) const override;
    unsigned int SetInteger(unsigned int which, uint32_t value) override;
    std::string GetString(unsigned int which) const override;
    unsigned int SetString(unsigned int which,
                           const std::string& value) override;
};

/** A recordset that presents the root item (with merged children).
 */
class Database::RootRecordset final: public db::DelegatingRecordset
{
    Database::Impl *m_database;
    mutable std::string m_children; ///< Calculated lazily

public:
    RootRecordset(db::RecordsetPtr rs, Database::Impl *database)
	: db::DelegatingRecordset(rs), m_database(database)
    {
    }

    std::string GetString(unsigned int which) const override;
    unsigned int SetString(unsigned int which,
                           const std::string& value) override;
};


		     /* Database::Impl */


Database::Impl::Impl()
{
}

Database::Impl::~Impl()
{
}

unsigned int Database::Impl::AddDatabase(mediadb::Database *db)
{
    Lock lock(this);
    for (size_t i=0; i<m_databases.size(); ++i)
    {
	if (!m_databases[i])
	{
	    m_databases[i] = db;
	    return 0;
	}
    }

    if (m_databases.size() >= 256)
	return ENOSPC;

    m_databases.push_back(db);
    return 0;
}

unsigned int Database::Impl::RemoveDatabase(mediadb::Database *db)
{
    Lock lock(this);
    for (size_t i=0; i<m_databases.size(); ++i)
    {
	if (m_databases[i] == db)
	{
	    m_databases[i] = NULL;
	    return 0;
	}
    }

    return 0;
}

unsigned int Database::Impl::AllocateID()
{
    Lock lock(this);
    if (m_databases.empty() || !m_databases[0])
	return 0;
    return m_databases[0]->AllocateID();
}

std::string Database::Impl::GetURL(unsigned int id)
{
    unsigned int db = id >> 24;

    Lock lock(this);
    if (db >= m_databases.size())
	return "";
    if (!m_databases[db])
	return "";
    return m_databases[db]->GetURL(id & 0xFFFFFF);
}

std::unique_ptr<util::Stream> Database::Impl::OpenRead(unsigned int id)
{
    unsigned int db = id >> 24;

    Lock lock(this);
    if (db >= m_databases.size())
	return std::unique_ptr<util::Stream>();
    if (!m_databases[db])
	return std::unique_ptr<util::Stream>();
    return m_databases[db]->OpenRead(id & 0xFFFFFF);
}

std::unique_ptr<util::Stream> Database::Impl::OpenWrite(unsigned int id)
{
    unsigned int db = id >> 24;

    Lock lock(this);
    if (db >= m_databases.size())
	return std::unique_ptr<util::Stream>();
    if (!m_databases[db])
	return std::unique_ptr<util::Stream>();
    return m_databases[db]->OpenWrite(id & 0xFFFFFF);
}

db::RecordsetPtr Database::Impl::CreateRecordset()
{
    Lock lock(this);
    if (m_databases.empty() || !m_databases[0])
	return db::RecordsetPtr(new db::EmptyRecordset);
    return m_databases[0]->CreateRecordset();
}

db::QueryPtr Database::Impl::CreateQuery()
{
    return db::QueryPtr(new Query(this));
}


        /* Database::Query */


db::RecordsetPtr Database::Query::Execute()
{
    Database::Impl::Lock lock(m_database);

    if (m_database->m_databases.empty() || !m_database->m_databases[0])
	return db::RecordsetPtr(new db::EmptyRecordset);

//    TRACE << "dbmerge: " << ToString() << "\n";

    /* Is it a simple "ID==n" query?
     */
    if (m_restrictions.size() == 1
	&& m_restrictions[0].which == mediadb::ID
	&& m_restrictions[0].rt == db::EQ)
    {
	unsigned int id = m_restrictions[0].ival;

	unsigned int dbno = id >> 24;
	if (dbno >= m_database->m_databases.size())
	    return db::RecordsetPtr();
	if (!m_database->m_databases[dbno])
	    return db::RecordsetPtr();

	if (dbno != 0 || id == mediadb::BROWSE_ROOT)
	{
	    /* Redirect to target database */
	    db::QueryPtr qp = m_database->m_databases[dbno]->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id & 0xFFFFFF));
	    db::RecordsetPtr rs = qp->Execute();
	    if (rs)
	    {
		if (id == mediadb::BROWSE_ROOT)
		    rs = db::RecordsetPtr(new RootRecordset(rs, m_database));
		else
		    rs = db::RecordsetPtr(new WrapRecordset(rs, dbno));
	    }
	    return rs;
	}
    }

    /* Not a special one, so just apply it to database 0 */
    db::QueryPtr qp = m_database->m_databases[0]->CreateQuery();
    unsigned int rc = qp->Clone(this);
    if (rc)
	return db::RecordsetPtr();
    db::RecordsetPtr rs = qp->Execute();
    return rs;
}


        /* Database::WrapRecordset */


uint32_t Database::WrapRecordset::GetInteger(unsigned int which) const
{
    unsigned int result = m_rs->GetInteger(which);
    if ( (which == mediadb::ID
	  || which == mediadb::IDPARENT
	  || which == mediadb::IDHIGH)
	 && result != mediadb::BROWSE_ROOT
	 && result != 0)
    {
	result |= (m_id << 24);
    }
    return result;
}

std::string Database::WrapRecordset::GetString(unsigned int which) const
{
    std::string result = m_rs->GetString(which);
    if (which == mediadb::CHILDREN && !result.empty())
    {
	std::vector<unsigned int> children;
	mediadb::ChildrenToVector(result, &children);
	for (unsigned int i=0; i<children.size(); ++i)
	    children[i] |= (m_id << 24);
	result = mediadb::VectorToChildren(children);
    }
    return result;
}

unsigned int Database::WrapRecordset::SetInteger(unsigned int which,
						 uint32_t value)
{
    if (which == mediadb::ID
	|| which == mediadb::IDHIGH
	|| which == mediadb::IDPARENT)
	value &= 0xFFFFFF;

    return m_rs->SetInteger(which, value);
}

unsigned int Database::WrapRecordset::SetString(unsigned int which,
						const std::string& value)
{
    if (which != mediadb::CHILDREN || value.empty())
	return m_rs->SetString(which, value);

    std::vector<unsigned int> children;
    mediadb::ChildrenToVector(value, &children);
    for (unsigned int i=0; i<children.size(); ++i)
	children[i] &= 0xFFFFFF;
    return m_rs->SetString(which, mediadb::VectorToChildren(children));
}


        /* Database::RootRecordset */


std::string Database::RootRecordset::GetString(unsigned int which) const
{
    if (which != mediadb::CHILDREN || m_rs->IsEOF())
	return m_rs->GetString(which);

    if (!m_children.empty())
	return m_children;

    Database::Impl::Lock lock(m_database);

    std::vector<unsigned int> children;

    for (unsigned int i=0; i<m_database->m_databases.size(); ++i)
    {
	if (m_database->m_databases[i])
	{
	    db::QueryPtr qp = m_database->m_databases[i]->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, mediadb::BROWSE_ROOT));
	    db::RecordsetPtr rs = qp->Execute();
	    if (rs && !rs->IsEOF())
	    {
		std::vector<unsigned int> these_children;
		mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN),
					  &these_children);
		
		for (unsigned int j=0; j<these_children.size(); ++j)
		    these_children[j] |= (i<<24);

		children.insert(children.end(),
				these_children.begin(),
				these_children.end());
	    }
	}
    }

    m_children = mediadb::VectorToChildren(children);
    return m_children;
}

unsigned int Database::RootRecordset::SetString(unsigned int which,
						const std::string& value)
{
    if (which != mediadb::CHILDREN)
	return m_rs->SetString(which, value);

    std::vector<unsigned int> children;
    mediadb::ChildrenToVector(value, &children);

    // Remove from the list anything >0xFFFFFF
    children.erase(std::remove_if(children.begin(),
				  children.end(),
				  std::bind2nd(std::greater<unsigned int>(),
					       0xFFFFFF)),
		   children.end());
    return m_rs->SetString(which, mediadb::VectorToChildren(children));
}


        /* Database itself */


Database::Database()
    : m_impl(new Impl())
{
}

Database::~Database()
{
    delete m_impl;
}

unsigned int Database::AddDatabase(mediadb::Database *db)
{
    return m_impl->AddDatabase(db);
}

unsigned int Database::RemoveDatabase(mediadb::Database *db)
{
    return m_impl->RemoveDatabase(db);
}

unsigned int Database::AllocateID()
{
    return m_impl->AllocateID();
}

std::string Database::GetURL(unsigned int id)
{
    return m_impl->GetURL(id);
}

std::unique_ptr<util::Stream> Database::OpenRead(unsigned int id)
{
    return m_impl->OpenRead(id);
}

std::unique_ptr<util::Stream> Database::OpenWrite(unsigned int id)
{
    return m_impl->OpenWrite(id);
}

db::QueryPtr Database::CreateQuery()
{
    return m_impl->CreateQuery();
}

db::RecordsetPtr Database::CreateRecordset()
{
    return m_impl->CreateRecordset();
}

} // namespace db::merge
} // namespace db

#ifdef TEST

# include "libmediadb/fake_database.h"

int main()
{
    mediadb::FakeDatabase db1, db2;

    db::RecordsetPtr rs = db1.CreateRecordset();
    rs->AddRecord();
    rs->SetInteger(mediadb::ID, mediadb::BROWSE_ROOT);
    rs->SetString(mediadb::TITLE, "root1");
    std::vector<unsigned int> children;
    children.push_back(0x120);
    rs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(children));
    rs->Commit();
    rs->AddRecord();
    rs->SetInteger(mediadb::ID, 0x120);
    rs->SetString(mediadb::TITLE, "Artists");
    rs->Commit();

    rs = db2.CreateRecordset();
    rs->AddRecord();
    rs->SetInteger(mediadb::ID, mediadb::BROWSE_ROOT);
    rs->SetString(mediadb::TITLE, "root2");
    children.clear();
    children.push_back(0x200);
    rs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(children));
    rs->Commit();
    rs->AddRecord();
    rs->SetInteger(mediadb::ID, 0x200);
    rs->SetString(mediadb::TITLE, "Radio");
    rs->Commit();

    db::merge::Database mdb;

    // Check it doesn't fall over with 0 databases
    db::QueryPtr qp = mdb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, mediadb::BROWSE_ROOT));
    rs = qp->Execute();
    assert(rs && rs->IsEOF());
    qp = mdb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 27));
    rs = qp->Execute();
    assert(rs && rs->IsEOF());
    qp = mdb.CreateQuery();
    qp->CollateBy(mediadb::ARTIST);
    rs = qp->Execute();
    assert(rs && rs->IsEOF());

    mdb.AddDatabase(&db1);
    
    // Check it's transparent to 1 database properly
    qp = mdb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, mediadb::BROWSE_ROOT));
    rs = qp->Execute();
    assert(rs && !rs->IsEOF());
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &children);
    assert(children.size() == 1);
    assert(children[0] = 0x120);

    mdb.AddDatabase(&db2);
    
    // Check it does its actual job with 2 databases
    qp = mdb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, mediadb::BROWSE_ROOT));
    rs = qp->Execute();
    assert(rs && !rs->IsEOF());
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &children);
    assert(children.size() == 2);
    assert(children[0] == 0x00000120);
    assert(children[1] == 0x01000200);
    children.push_back(0x120);
    rs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(children));
    rs->MoveNext();
    assert(rs->IsEOF());
    qp = mdb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 0x01000200));
    rs = qp->Execute();
    assert(rs && !rs->IsEOF());
    assert(rs->GetInteger(mediadb::ID) == 0x01000200);
    qp = mdb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 0x00000120));
    rs = qp->Execute();
    assert(rs && !rs->IsEOF());
    assert(rs->GetInteger(mediadb::ID) == 0x00000120);

    // Check that first db has correctly-rewritten root
    qp = db1.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, mediadb::BROWSE_ROOT));
    rs = qp->Execute();
    assert(rs && !rs->IsEOF());
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &children);    
    assert(children.size() == 2);
    assert(children[0] == 0x120);
    assert(children[1] == 0x120);

    return 0;
}

#endif
