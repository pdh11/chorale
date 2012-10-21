#include "config.h"
#include "db.h"
#include "query.h"
#include "rs.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/ssdp.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include "libupnp/ContentDirectory2_client.h"
#include "libmediadb/schema.h"
#include <errno.h>

#ifdef HAVE_UPNP

namespace db {
namespace upnpav {

Database::Database()
    : m_search_caps(0)
{
}

Database::~Database()
{
}

unsigned Database::Init(const std::string& url, const std::string& udn)
{
    unsigned int rc = m_upnp.Init(url, udn);
    if (rc != 0)
	return rc;

    m_contentdirectory.Init(&m_upnp, util::ssdp::s_uuid_contentdirectory);

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
	TRACE << "SearchCaps='" << sc << "'\n";
    else
	TRACE << "Can't GetSearchCapabilities: " << rc << "\n";

    if (sc == "*" || strstr(sc.c_str(), "upnp:class"))
    {
	std::string result;
	rc = m_contentdirectory.Browse("0",
				       upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA,
				       "upnp:searchClass", 0, 0, "",
				       &result, NULL, NULL, NULL);
	if (rc == 0)
	{
	    TRACE << "searchClasses=" << result << "\n";
	    
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
    return m_upnp.GetDescription().GetFriendlyName();
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

#endif // HAVE_UPNP


        /* Unit tests */


#ifdef TEST

#include "libmediadb/schema.h"

class MyCallback: public util::ssdp::Client::Callback
{
public:
    void OnService(const std::string& url, const std::string& udn);
};

void MyCallback::OnService(const std::string& url, const std::string& udn)
{
#ifdef HAVE_UPNP
    db::upnpav::Database thedb;
    thedb.Init(url, udn);
#endif
}

int main()
{
#ifdef HAVE_UPNP
    util::LibUPnPUser uuu;
    db::upnpav::Database thedb;
//    thedb.Init("http://10.35.1.65:50539/");
/*
    db::QueryPtr qp = thedb.CreateQuery();
    qp->Restrict(mediadb::ID, db::EQ, 0x100);
    db::RecordsetPtr rs = qp->Execute();
    std::string children = rs->GetString(mediadb::CHILDREN);
    TRACE << "root is called " << rs->GetString(mediadb::TITLE) << "\n";
    */
    return 0;

    util::ssdp::Client client(NULL);
    MyCallback cb;
    client.Init(util::ssdp::s_uuid_contentdirectory, &cb);

    sleep(25);
    
#endif // HAVE_UPNP

    return 0;
}
#endif // TEST
