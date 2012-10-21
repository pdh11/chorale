#include "config.h"
#include "db.h"
#include "query.h"
#include "rs.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/ssdp.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include "libmediadb/schema.h"
#include <errno.h>

#ifdef HAVE_UPNP

namespace db {
namespace upnpav {

unsigned Database::Init(const std::string& url)
{
    m_description_url = url;

    upnp::Description desc;
    unsigned rc = desc.Fetch(url);
    if (rc != 0)
    {
	TRACE << "Can't Fetch()\n";
	return rc;
    }

    m_friendly_name = desc.GetFriendlyName();
    m_udn = desc.GetUDN();

    const upnp::Services& svc = desc.GetServices();

    upnp::Services::const_iterator it
	= svc.find(util::ssdp::s_uuid_contentdirectory);
    if (it == svc.end())
    {
	TRACE << "No ContentDirectory service\n";
	return ENOENT;
    }
    
    TRACE << "Content directory is at " << it->second.control_url << "\n";
    m_control_url = it->second.control_url;

    // Set up root item
    m_idmap[0x100] = "0";
    m_revidmap["0"] = 0x100;
    m_nextid = 0x110;

    upnp::soap::Connection usc(m_control_url,
			       m_udn,
			       util::ssdp::s_uuid_contentdirectory);

/*
    upnp::soap::Params in;

    upnp::soap::Params out = usc.Action("GetSearchCapabilities", in);
    in["ContainerID"] = "0";
    in["SearchCriteria"] = "upnp:class derivedfrom \"object.container\"";
    in["Filter"] = "*";
    in["StartingIndex"] = "0";
    in["RequestedCount"] = "0";
    in["SortCriteria"] = "";
    
    out = usc.Action("Search", in);
*/
    return 0;
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
    revidmap_t::const_iterator it = m_revidmap.find(objectid);
    if (it != m_revidmap.end())
	return it->second;

    unsigned int id = m_nextid++;
    m_idmap[id] = objectid;
    m_revidmap[objectid] = id;
    return id;
}

}; // namespace upnpav
}; // namespace db

#endif // HAVE_UPNP


        /* Unit tests */


#ifdef TEST

#include "libmediadb/schema.h"

class MyCallback: public util::ssdp::Client::Callback
{
public:
    void OnService(const std::string& url);
};

void MyCallback::OnService(const std::string& url)
{
#ifdef HAVE_UPNP
    db::upnpav::Database thedb;
    thedb.Init(url);
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

    sleep(5);
    
#endif // HAVE_UPNP

    return 0;
}
#endif // TEST
