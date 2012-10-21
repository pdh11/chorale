#include "config.h"
#include "db.h"
#include "query.h"
#include "rs.h"
#include "libutil/trace.h"
#include "libupnp/ssdp.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include "libupnp/ContentDirectory3_client.h"
#include "libmediadb/schema.h"
#include <errno.h>
#include <string.h>

namespace db {
namespace upnpav {

Database::Database(util::http::Client *client,
		   util::http::Server *server,
		   util::Scheduler *poller)
    : m_device_client(client, server, poller),
      m_contentdirectory(&m_device_client, 
			 upnp::s_service_id_content_directory),
      m_nextid(0x110),
      m_search_caps(0)
{
}

Database::~Database()
{
}

unsigned Database::Init(const std::string& url, const std::string& udn)
{
    unsigned int rc = m_device_client.Init(url, udn);
    if (rc)
	return rc;

    rc = m_contentdirectory.Init();
    if (rc)
	return rc;

    {
	Lock lock(this);

	// Set up root item
	m_idmap[0x100] = "0";
	m_revidmap["0"] = 0x100;
	m_nextid = 0x110;
    }

    std::string sc;
    rc = m_contentdirectory.GetSearchCapabilities(&sc);
    if (rc == 0)
    {
//	TRACE << "SearchCaps='" << sc << "'\n";
    }
    else
	TRACE << "Can't GetSearchCapabilities: " << rc << "\n";

    if (sc == "*" || strstr(sc.c_str(), "upnp:class"))
    {
	std::string result;
	rc = m_contentdirectory.Browse("0",
				       upnp::ContentDirectory3::BROWSEFLAG_BROWSE_METADATA,
				       "upnp:searchClass", 0, 0, "",
				       &result, NULL, NULL, NULL);
	if (rc == 0)
	{
//	    TRACE << "searchClasses=" << result << "\n";
	    
	    if (strstr(result.c_str(), "object.container.person.musicArtist")
		&& (sc == "*" || strstr(sc.c_str(), "upnp:artist")))
		m_search_caps |= (1<<mediadb::ARTIST);
	    if (strstr(result.c_str(), "object.container.album.musicAlbum")
		&& (sc == "*" || strstr(sc.c_str(), "upnp:album")))
		m_search_caps |= (1<<mediadb::ALBUM);
	    if (strstr(result.c_str(), "object.container.genre.musicGenre")
		&& (sc == "*" || strstr(sc.c_str(), "upnp:genre")))
		m_search_caps |= (1<<mediadb::GENRE);
	}
	else
	{
	    TRACE << "Can't Browse root item: " << rc << "\n";
	}
    }

    return 0;
}

const std::string& Database::GetFriendlyName() const
{
    return m_device_client.GetFriendlyName();
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr();
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(this));
}

std::string Database::GetURL(unsigned int id)
{
    RecordsetOne rs(this, id);
    return rs.GetString(mediadb::PATH);
}

util::SeekableStreamPtr Database::OpenRead(unsigned int)
{
    return util::SeekableStreamPtr();
}

util::SeekableStreamPtr Database::OpenWrite(unsigned int)
{
    return util::SeekableStreamPtr();
}

unsigned int Database::IdForObjectId(const std::string& objectid)
{
    Lock lock(this);

    revidmap_t::const_iterator it = m_revidmap.find(objectid);
    if (it != m_revidmap.end())
	return it->second;

    unsigned int id = m_nextid++;
    m_idmap[id] = objectid;
    m_revidmap[objectid] = id;
    return id;
}

std::string Database::ObjectIdForId(unsigned int id)
{
    Lock lock(this);

    idmap_t::const_iterator it = m_idmap.find(id);
    if (it != m_idmap.end())
	return it->second;

    // This way round should never be the first time we ask the question

    TRACE << "Unknown id " << id << "\n";
    return std::string();
}

} // namespace upnpav
} // namespace db


        /* Unit tests */


#ifdef TEST

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

    db::upnpav::Database udb(&wc, &ws, &poller);
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
