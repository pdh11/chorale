#include "config.h"
#include "database.h"
#include "query.h"
#include "recordset.h"
#include "libutil/trace.h"
#include "libmediadb/schema.h"
#include "libutil/stream.h"
#include <errno.h>
#include <string.h>

namespace db {
namespace upnp {

Database::Database(util::http::Client *client,
		   util::http::Server *server,
		   util::Scheduler *poller)
    : m_connection(client, server, poller)
{
}

Database::~Database()
{
}

unsigned Database::Init(const std::string& url, const std::string& udn)
{
    return m_connection.Init(url, udn);
}

unsigned Database::Init(const std::string& url, const std::string& udn,
			InitCallback cb)
{
    return m_connection.Init(url, udn, cb);
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr();
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(&m_connection));
}

std::string Database::GetURL(unsigned int id)
{
    RecordsetOne rs(&m_connection, id);
    return rs.GetString(mediadb::PATH);
}

std::unique_ptr<util::Stream> Database::OpenRead(unsigned int)
{
    return std::unique_ptr<util::Stream>();
}

std::unique_ptr<util::Stream> Database::OpenWrite(unsigned int)
{
    return std::unique_ptr<util::Stream>();
}

} // namespace upnpav
} // namespace db


        /* Unit tests */


#if 0 //def TEST

# include "libutil/scheduler.h"
# include "libutil/http_client.h"
# include "libutil/http_server.h"
# include "libutil/worker_thread_pool.h"
# include "libdblocal/db.h"
# include "libupnp/ssdp.h"
# include "libupnp/server.h"
# include "libupnpd/media_server.h"
# include "libmediadb/schema.h"
# include "libmediadb/xml.h"
# include "libdbsteam/db.h"
# include <boost/format.hpp>

int main()
{
    // Create database

    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ARTIST,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ALBUM,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::GENRE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::TITLE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    mediadb::ReadXML(&sdb, SRCROOT "/libmediadb/example.xml");

    util::http::Client wc;
    db::local::Database mdb(&sdb, &wc);

    // Create UPnP MediaServer for that database

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);
    util::BackgroundScheduler poller;
    util::http::Server ws(&poller, &wtp);
    upnp::ssdp::Responder ssdp(&poller, NULL);

    ws.Init();
    wtp.PushTask(util::SchedulerTask::Create(&poller));

    upnp::Server server(&poller, &wc, &ws, &ssdp);
    upnpd::MediaServer ms(&mdb, NULL);
    unsigned int rc = ms.Init(&server, "tests");
    assert(rc == 0);
    rc = server.Init();
    assert(rc == 0);

    std::string descurl = (boost::format("http://127.0.0.1:%u/upnp/description.xml")
			      % ws.GetPort()).str();

    // Create db::upnpav::Database connected to that media server

    db::upnp::Database udb(&wc, &ws, &poller);
    rc = udb.Init(descurl, ms.GetUDN());
    assert(rc == 0);

    db::QueryPtr qp = udb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 0x100));
    db::RecordsetPtr rs = qp->Execute();
    assert(rs);
    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::TITLE) == "mp3");
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);
    std::vector<unsigned int> children;
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &children);
    assert(children.size() == 5);

    qp = udb.CreateQuery();
    rc = qp->CollateBy(mediadb::ARTIST);
    assert(rc == 0);
    rs = qp->Execute();
    assert(rs);
    assert(!rs->IsEOF());
    assert(rs->GetString(0) == "Depeche Mode");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(0) == "Nine Black Alps");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(0) == "Underworld");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(0) == "Zero 7");
    rs->MoveNext();
    assert(rs->IsEOF());
    
    // Check that it knows it *can't* do this one (which is a shame)
    qp = udb.CreateQuery();
    rc = qp->CollateBy(mediadb::YEAR);
    assert(rc != 0);

    return 0;
}
#endif // TEST
