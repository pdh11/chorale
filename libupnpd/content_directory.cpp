#include "config.h"
#include "content_directory.h"
#include "libmediadb/localdb.h"
#include "libmediadb/schema.h"
#include "libmediadb/xml.h"
#include "libdbsteam/db.h"
#include "libupnp/didl.h"
#include "libutil/trace.h"
#include "libutil/poll.h"
#include "libutil/web_server.h"
#include "libupnp/ContentDirectory2_client.h"
#include "media_server.h"
#include "libupnp/device.h"
#include <sstream>
#include <errno.h>
#include <boost/format.hpp>

#ifdef HAVE_UPNP

#include <upnp/upnp.h>

namespace upnpd {

ContentDirectoryImpl::ContentDirectoryImpl(mediadb::Database *db,
					   unsigned short port)
    : m_db(db),
      m_port(port)
{
}

unsigned int ContentDirectoryImpl::Browse(const std::string& ObjectID,
					  const std::string& BrowseFlag,
					  const std::string& /*Filter*/,
					  uint32_t StartingIndex,
					  uint32_t RequestedCount,
					  const std::string& /*SortCriteria*/,
					  std::string *Result,
					  uint32_t *NumberReturned,
					  uint32_t *TotalMatches,
					  uint32_t *UpdateID)
{
    unsigned int id = strtoul(ObjectID.c_str(), NULL, 10);

    db::QueryPtr qp = m_db->CreateQuery();
    unsigned int rc = qp->Restrict(mediadb::ID, db::EQ, id ? id : 0x100);
    if (rc != 0)
	return rc;
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	return ENOENT;
    }

    std::vector<unsigned int> children;
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &children);

    std::ostringstream ss;
    ss << upnp::didl::s_header;

    /** @todo Determine IP address to use from socket (or SoapInfo structure)
     */
    std::string urlprefix;
    if (m_port)
    {
	urlprefix = (boost::format("http://%s:%u/content/")
		     % UpnpGetServerIpAddress()
		     % m_port).str();
    }

    if (BrowseFlag == "BrowseMetadata")
    {
	ss << upnp::didl::FromRecord(m_db, rs, urlprefix.c_str());
	*NumberReturned = 1;
	*TotalMatches = 1;
	*UpdateID = 1;
    }
    else if (BrowseFlag == "BrowseDirectChildren")
    {
	if (StartingIndex >= children.size())
	    return ENOENT;

	if (RequestedCount == 0)
	    RequestedCount = children.size();
	
	if (StartingIndex + RequestedCount > children.size())
	    RequestedCount = children.size() - StartingIndex;

	for (unsigned int i = 0; i < RequestedCount; ++i)
	{
	    qp = m_db->CreateQuery();
	    qp->Restrict(mediadb::ID, db::EQ, children[StartingIndex+i]);
	    rs = qp->Execute();
	    if (rs && !rs->IsEOF())
		ss << upnp::didl::FromRecord(m_db, rs, urlprefix.c_str());
	}
	*NumberReturned = RequestedCount;
	*TotalMatches = children.size();
	*UpdateID = 1;
    }
    else
	return EINVAL;

    ss << upnp::didl::s_footer;

    *Result = ss.str();
    return 0;
}

}; // namespace upnpd

#endif // HAVE_UPNP


        /* Unit tests */


#ifdef TEST

static const struct {
    const char *objectid;
    const char *flag;
    const char *filter;
    unsigned int start;
    unsigned int requestcount;
    const char *sortby;

    const char *result;
    unsigned int n;
    unsigned int total;
} tests[] = {
    { "0", "BrowseMetadata", "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\"><container id=\"0\" parentID=\"-1\" restricted=\"1\" childCount=\"5\"><dc:title>mp3</dc:title><upnp:class>object.container.storageFolder</upnp:class><upnp:storageUsed>-1</upnp:storageUsed></container></DIDL-Lite>", 
      1, 1 },

    { "0", "BrowseDirectChildren", "*", 0, 0, "",

"<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
" xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"

"<container id=\"289\" parentID=\"0\" restricted=\"1\" childCount=\"17\">"
"<dc:title>1992-2002</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"290\" parentID=\"0\" restricted=\"1\" childCount=\"12\">"
"<dc:title>Everything Is</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"291\" parentID=\"0\" restricted=\"1\" childCount=\"4\">"
"<dc:title>SongsStartingWithB</dc:title>"
"<upnp:class>object.container.playlistContainer</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"293\" parentID=\"0\" restricted=\"1\" childCount=\"13\">"
"<dc:title>The Garden</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"294\" parentID=\"0\" restricted=\"1\" childCount=\"10\">"
"<dc:title>Violator</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

      "</DIDL-Lite>", 5, 5 }
};

enum { TESTS = sizeof(tests)/sizeof(tests[0]) };

int main(int, char**)
{
#ifdef HAVE_UPNP
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

    mediadb::LocalDatabase mdb(&sdb);

    upnpd::ContentDirectoryImpl cd(&mdb, 0);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	std::string result;
	uint32_t n;
	uint32_t total;
	uint32_t updateid;
	unsigned int rc = cd.Browse(tests[i].objectid,
				    tests[i].flag,
				    tests[i].filter,
				    tests[i].start,
				    tests[i].requestcount,
				    tests[i].sortby,

				    &result, &n, &total, &updateid);
	assert(rc == 0);
	assert(result == tests[i].result);
	assert(n == tests[i].n);
	assert(total == tests[i].total);
    }

    util::Poller poller;
    util::WebServer ws;

    upnpd::MediaServer ms(&mdb, ws.GetPort(), "tests");
    upnp::DeviceManager udm;
    unsigned int rc = udm.SetRootDevice(ms.GetDevice());
    assert(rc == 0);

    std::string controlurl = (boost::format("http://127.0.0.1:%u/upnpcontrol0")
			      % UpnpGetServerPort()).str();

    upnp::soap::Connection soap(controlurl, ms.GetDevice()->UDN(),
				"urn:schemas-upnp-org:service:ContentDirectory:1");
    upnp::ContentDirectory2Client cdc(&soap);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	std::string result;
	uint32_t n;
	uint32_t total;
	uint32_t updateid;
	rc = cdc.Browse(tests[i].objectid,
			tests[i].flag,
			tests[i].filter,
			tests[i].start,
			tests[i].requestcount,
			tests[i].sortby,
			
			&result, &n, &total, &updateid);
	assert(rc == 0);
	assert(result == tests[i].result);
	assert(n == tests[i].n);
	assert(total == tests[i].total);
    }
#endif // HAVE_UPNP

    return 0;
}

#endif // TEST
