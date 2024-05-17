#include "tag_serialiser.h"
#include "libutil/task.h"
#include "libutil/task_queue.h"
#include "libutil/trace.h"
#include "libutil/bind.h"
#include "libutil/counted_pointer.h"
#include "libimport/tags.h"
#include "libmediadb/schema.h"
#include "libmediadb/db.h"
#include "libdb/query.h"
#include "libdb/recordset.h"

namespace db {
namespace local {

class TagSerialiser::Task: public util::Task
{
    TagSerialiser *m_parent;
    unsigned int m_id;

    Task(TagSerialiser *parent, unsigned int id)
	: m_parent(parent),
	  m_id(id)
    {
    }

    typedef util::CountedPointer<Task> TaskPtr;

public:
    static util::TaskCallback Create(TagSerialiser *parent, unsigned int id);

    unsigned int Run();
};

util::TaskCallback TagSerialiser::Task::Create(TagSerialiser *parent, 
					       unsigned int id)
{
    return util::Bind(TaskPtr(new Task(parent, id))).To<&Task::Run>();
}

unsigned int TagSerialiser::Task::Run()
{
//    TRACE << "Rewrite task running for " << m_id << "\n";

    bool finished = false;
    
    {
        std::lock_guard<std::mutex> lock(m_parent->m_mutex);
	m_parent->m_need_rewrite.erase(m_id);
    }
    
    while (!finished)
    {
	db::QueryPtr qp = m_parent->m_database->CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, m_id));
	db::RecordsetPtr rs = qp->Execute();

	if (rs && !rs->IsEOF())
	{
	    import::TagWriter tags;
	    
	    unsigned int rc = tags.Init(rs->GetString(mediadb::PATH));
	    if (rc)
	    {
		TRACE << "Can't open " << rs->GetString(mediadb::PATH)
		      << ": " << rc << "\n";
	    }
	    else
	    {
		rc = tags.Write(rs.get());
		if (rc)
		{
		    TRACE << "Can't retag " << rs->GetString(mediadb::PATH)
			  << ": " << rc << "\n";
		}
		else
		{
//		    TRACE << "Tagged " << m_id << "\n";
		}
	    }
	}

	{
            std::lock_guard<std::mutex> lock(m_parent->m_mutex);

	    // Was another rewrite called-for while we were doing it?
	    if (m_parent->m_need_rewrite.find(m_id)
		!= m_parent->m_need_rewrite.end())
	    {
		finished = false;
		m_parent->m_need_rewrite.erase(m_id);
		// Go round again

//		TRACE << "Rewrite task for " << m_id << " goes round again\n";
	    }
	    else
	    {
		finished = true;
		m_parent->m_rewrite_tasks.erase(m_id); // Exit this task
	    }
	}
    }
    
//    TRACE << "Rewrite task for " << m_id << " exits\n";

    return 0;
}


TagSerialiser::TagSerialiser(mediadb::Database *database, 
			     util::TaskQueue *queue)
    : m_database(database),
      m_queue(queue)
{
}


unsigned int TagSerialiser::Rewrite(unsigned int id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_need_rewrite.find(id) != m_need_rewrite.end())
    {
//	TRACE << "Rewrite of " << id << " already queued\n";
	return 0;
    }

    m_need_rewrite.insert(id);

    if (m_rewrite_tasks.find(id) != m_rewrite_tasks.end())
    {
//	TRACE << "Existing rewrite task for " << id
//	      << " should take care of it\n";
	return 0;
    }

//    TRACE << "Creating new rewrite task for " << id << "\n";

    m_rewrite_tasks.insert(id);

    util::TaskCallback tc = Task::Create(this, id);
    m_queue->PushTask(tc);

    return 0;
}

} // namespace local
} // namespace db

#ifdef TEST

# include "libdbsteam/db.h"
# include "db.h"
# include "libutil/file.h"
# include "libutil/worker_thread_pool.h"
# include "libutil/http_client.h"
# include <stdlib.h>
# include <errno.h>
# include <stdio.h>
# include <boost/format.hpp>

int main()
{
    enum { FILES = 5, TAGS = 40 };

    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    char root[] = "tag_serialiser.test.XXXXXX";

    if (!mkdtemp(root))
    {
	fprintf(stderr, "Can't create temporary dir\n");
	return 1;
    }

    std::string fullname = util::Canonicalise(root);

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);

    util::http::Client client;
    db::local::Database ldb(&sdb, &client);

    db::local::TagSerialiser ts(&ldb, &wtp);

    db::RecordsetPtr rs = sdb.CreateRecordset();

    for (unsigned int i=0; i<FILES; ++i)
    {
	std::string filename = fullname
	    + (boost::format("/test%d.mp3") % i).str();
	FILE *f = fopen(filename.c_str(), "wb");
	fclose(f);

	rs->AddRecord();
	rs->SetInteger(mediadb::ID, 1000+i);
	rs->SetString(mediadb::PATH, filename);
	rs->Commit();
    }

    srand(42);

    for (unsigned int j=0; j<TAGS; ++j)
    {
	unsigned int recno = rand() % FILES;

	db::QueryPtr qp = sdb.CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, 1000 + recno));
	rs = qp->Execute();

	assert(rs && !rs->IsEOF());
	rs->SetInteger(mediadb::TITLE, j);
	rs->Commit();
	ts.Rewrite(1000 + recno);
    }

    while (wtp.Count())
    {
	TRACE << wtp.Count() << " tasks, sleeping\n";
	sleep(1);
    }

    std::string rmrf = "rm -r " + fullname;
    if (system(rmrf.c_str()) < 0)
    {
	fprintf(stderr, "Can't tidy up: %d\n", errno);
	return 1;
    }

    return 0;
}

#endif
