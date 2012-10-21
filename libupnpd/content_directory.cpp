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
#include "libutil/xmlescape.h"
#include "libutil/urlescape.h"
#include "libupnp/ContentDirectory2_client.h"
#include "media_server.h"
#include "search.h"
#include "libupnp/server.h"
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

    TRACE << BrowseFlag << "(" << ObjectID << ")\n";

    db::QueryPtr qp = m_db->CreateQuery();
    unsigned int rc = qp->Where(qp->Restrict(mediadb::ID, db::EQ,
					     id ? id : 0x100));
    if (rc != 0)
    {
	TRACE << "Can't do query\n";
	return rc;
    }
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "ID " << ObjectID << " not found\n";
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
	if (StartingIndex > children.size())
	{
	    TRACE << "StartingIndex " << StartingIndex << " off the end ("
		  << children.size() << ")\n";
	    return ENOENT;
	}

	if (RequestedCount == 0)
	    RequestedCount = children.size();
	
	if (StartingIndex + RequestedCount > children.size())
	    RequestedCount = children.size() - StartingIndex;

	for (unsigned int i = 0; i < RequestedCount; ++i)
	{
	    qp = m_db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 
				   children[StartingIndex+i]));
	    rs = qp->Execute();
	    if (rs && !rs->IsEOF())
		ss << upnp::didl::FromRecord(m_db, rs, urlprefix.c_str());
	}
	*NumberReturned = RequestedCount;
	*TotalMatches = children.size();
	*UpdateID = 1;
    }
    else
    {
	TRACE << "Don't like browse flag '" << BrowseFlag << "'\n";
	return EINVAL;
    }

    ss << upnp::didl::s_footer;

    TRACE << "Browse result is " << ss.str() << "\n\n";

    *Result = ss.str();
    return 0;
}

unsigned int ContentDirectoryImpl::Search(const std::string& ContainerID,
					  const std::string& SearchCriteria,
					  const std::string& /*Filter*/,
					  uint32_t StartingIndex,
					  uint32_t RequestedCount,
					  const std::string& /*SortCriteria*/,
					  std::string *Result,
					  uint32_t *NumberReturned,
					  uint32_t *TotalMatches,
					  uint32_t *UpdateID)
{
    TRACE << "Search(" << SearchCriteria << ")\n";

    db::QueryPtr qp = m_db->CreateQuery();

    unsigned int collate = 0;

    unsigned int rc = ApplySearchCriteria(qp, SearchCriteria, &collate);
    if (rc != 0)
	return rc;

    db::RecordsetPtr rs = qp->Execute();

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

    unsigned int n = 0;
    unsigned int nok = 0;

    if (RequestedCount == 0)
	RequestedCount = (uint32_t)-1;

    while (rs && !rs->IsEOF() && nok < RequestedCount)
    {
	if (n >= StartingIndex)
	{
	    if (collate)
	    {
		/** These object IDs never come back to us in Browse, so it's
		 * OK to use fictitious ones.
		 */
		ss << "<container id=\"" << collate << ":" << n
		   << "\" parentID=\"" << ContainerID << "\" restricted=\"1\""
		    " searchable=\"1\">"
		   << "<dc:title>" << util::XmlEscape(rs->GetString(0))
		   << "</dc:title>";

		/** Note that the searchClass values here are what decide the
		 * menu hierarchy on the client. Artist leads to Album,
		 * Album leads to tracks, Genre leads to tracks. (And that's
		 * all you get -- you can't directly express decade or year
		 * queries as container objects.)
		 */
		switch (collate)
		{
		case mediadb::ARTIST:
		    ss << "<upnp:class>object.container.person.musicArtist"
			"</upnp:class>"
			"<upnp:artist>" << util::XmlEscape(rs->GetString(0))
		       << "</upnp:artist>"
			"<upnp:searchClass includeDerived=\"0\">"
			"object.container.album.musicAlbum"
			"</upnp:searchClass>";
		    break;
		case mediadb::ALBUM:
		    ss << "<upnp:class>object.container.album.musicAlbum"
			"</upnp:class>"
			"<upnp:album>" << util::XmlEscape(rs->GetString(0))
		       << "</upnp:album>"
			"<upnp:searchClass includeDerived=\"0\">"
			"object.item.audioItem.musicTrack"
			"</upnp:searchClass>";
		    break;
		case mediadb::GENRE:
		    ss << "<upnp:class>object.container.genre.musicGenre"
			"</upnp:class>"
			"<upnp:genre>" << util::XmlEscape(rs->GetString(0))
		       << "</upnp:genre>"
			"<upnp:searchClass includeDerived=\"0\">"
			"object.item.audioItem.musicTrack"
			"</upnp:searchClass>";
		    break;
		}
		ss << "</container>";
	    }
	    else
	    {
		ss << upnp::didl::FromRecord(m_db, rs, urlprefix.c_str());
	    }
	    ++nok;
	}

	rs->MoveNext();
	++n;
    }
    ss << upnp::didl::s_footer;

//    TRACE << "Search result is " << ss.str() << " (" << nok << " items)\n";

    *Result = ss.str();
    *NumberReturned = nok;
    *TotalMatches = (!rs || rs->IsEOF()) ? n : 0;
    *UpdateID = 1;

    return 0;
}

unsigned int ContentDirectoryImpl::GetSearchCapabilities(std::string *ps)
{
    /** This is overstating matters slightly -- we only really support
     *            "upnp:class,upnp:artist,upnp:album,@id,@parentID,
     *            dc:title,upnp:genre,dc:date"
     * -- but Soundbridge won't offer searches unless we claim "*".
     */
    *ps = "*";
    return 0;
}

unsigned int ContentDirectoryImpl::GetSortCapabilities(std::string *ps)
{
    *ps = std::string();
    return 0;
}

} // namespace upnpd

#endif // HAVE_UPNP


        /* Unit tests */


#ifdef TEST

# include "libdbupnp/db.h"
# include "libutil/ssdp.h"

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
} browsetests[] = {
    { "0", "BrowseMetadata", "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"1\" childCount=\"5\""
      " searchable=\"1\">"
      "<dc:title>mp3</dc:title>"
      "<upnp:class>object.container.storageFolder</upnp:class>"
      "<upnp:storageUsed>-1</upnp:storageUsed>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum</upnp:searchClass>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.genre.musicGenre</upnp:searchClass>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.person.musicArtist</upnp:searchClass>"
      "</container></DIDL-Lite>",
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

enum { BROWSETESTS = sizeof(browsetests)/sizeof(browsetests[0]) };

static const struct {
    const char *objectid;
    const char *criteria;
    const char *filter;
    unsigned int start;
    unsigned int requestcount;
    const char *sortby;

    const char *result;
    unsigned int n;
    unsigned int total;
} searchtests[] = {

    { "0", "dc:title = \"SongsStartingWithB\"", "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"

      "<container id=\"291\" parentID=\"0\" restricted=\"1\" childCount=\"4\">"
      "<dc:title>SongsStartingWithB</dc:title>"
      "<upnp:class>object.container.playlistContainer</upnp:class>"
      "<upnp:storageUsed>-1</upnp:storageUsed>"
      "</container>"

      "</DIDL-Lite>", 1, 1 },

    // This is the Soundbridge playlists menu query
    { "0", "upnp:class = \"object.container.playlistContainer\" and @refID exists false", "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"291\" parentID=\"0\" restricted=\"1\" childCount=\"4\">"
      "<dc:title>SongsStartingWithB</dc:title>"
      "<upnp:class>object.container.playlistContainer</upnp:class>"
      "<upnp:storageUsed>-1</upnp:storageUsed>"
      "</container>"

      "</DIDL-Lite>", 1, 1 },

    // This is a Soundbridge search-titles query
    { "0",
      "upnp:class derivedfrom \"object.item.audioItem\" "
      "and @refID exists false and dc:title contains \"ad\"", "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"353\" parentID=\"290\" restricted=\"1\">"
      "<dc:title>Headlights</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Nine Black Alps</upnp:artist>"
      "<upnp:album>Everything Is</upnp:album>"
      "<upnp:originalTrackNumber>5</upnp:originalTrackNumber>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>2005-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"4209148\""
      " duration=\"0:02:44.00\">161</res>"
      "<res protocolInfo=\"http-get:*:audio/x-flac:*\" size=\"22115187\""
      " duration=\"0:02:44.00\">162</res>"
      "</item>"
      "</DIDL-Lite>", 1, 1 },

    // This is the Soundbridge artists menu query
    { "0",
      "upnp:class = \"object.container.person.musicArtist\""
      " and @refID exists false", "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"2:0\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>Depeche Mode</dc:title>"
      "<upnp:class>object.container.person.musicArtist</upnp:class>"
      "<upnp:artist>Depeche Mode</upnp:artist>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum"
      "</upnp:searchClass>"
      "</container>"
      "<container id=\"2:1\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>Nine Black Alps</dc:title>"
      "<upnp:class>object.container.person.musicArtist</upnp:class>"
      "<upnp:artist>Nine Black Alps</upnp:artist>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum"
      "</upnp:searchClass>"
      "</container>"
      "<container id=\"2:2\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>Underworld</dc:title>"
      "<upnp:class>object.container.person.musicArtist</upnp:class>"
      "<upnp:artist>Underworld</upnp:artist>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum"
      "</upnp:searchClass>"
      "</container>"
      "<container id=\"2:3\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>Zero 7</dc:title>"
      "<upnp:class>object.container.person.musicArtist</upnp:class>"
      "<upnp:artist>Zero 7</upnp:artist>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum"
      "</upnp:searchClass>"
      "</container>"
      "</DIDL-Lite>", 4, 4 },
    
    // This is the Soundbridge artist/album query
    { "0",
      "upnp:class = \"object.container.album.musicAlbum\""
      " and upnp:artist = \"Depeche Mode\" and @refID exists false",
      "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"3:0\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>Violator</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>Violator</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.item.audioItem.musicTrack"
      "</upnp:searchClass>"
      "</container>"
      "</DIDL-Lite>", 1, 1 },

    // Leaf query from artist/album menu (restricted to first 2 songs)
    { "0",
      "upnp:class derivedfrom \"object.item.audioItem\""
      " and @refID exists false and upnp:artist = \"Depeche Mode\""
      " and upnp:album = \"Violator\"",
      "*", 0, 2, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"331\" parentID=\"0\" restricted=\"1\">"
      "<dc:title>Blue Dress</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Depeche Mode</upnp:artist>"
      "<upnp:album>Violator</upnp:album>"
      "<upnp:originalTrackNumber>8</upnp:originalTrackNumber>"
      "<upnp:genre>Rock</upnp:genre>"
      "<dc:date>1990-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"9557588\""
      " duration=\"0:05:38.00\">14b</res>"
      "<res protocolInfo=\"http-get:*:audio/x-flac:*\" size=\"33977715\""
      " duration=\"0:05:38.00\">15f</res>"
      "</item>"
      "<item id=\"366\" parentID=\"0\" restricted=\"1\">"
      "<dc:title>World In My Eyes</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Depeche Mode</upnp:artist>"
      "<upnp:album>Violator</upnp:album>"
      "<upnp:originalTrackNumber>1</upnp:originalTrackNumber>"
      "<upnp:genre>Rock</upnp:genre>"
      "<dc:date>1990-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"7629896\""
      " duration=\"0:04:27.00\">"
      "16e"
      "</res>"
      "<res protocolInfo=\"http-get:*:audio/x-flac:*\" size=\"30686347\""
      " duration=\"0:04:27.00\">"
      "16f"
      "</res>"
      "</item>"
      "</DIDL-Lite>", 2, 0 },
};

enum { SEARCHTESTS = sizeof(searchtests)/sizeof(searchtests[0]) };


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

    for (unsigned int i=0; i<BROWSETESTS; ++i)
    {
	std::string result;
	uint32_t n;
	uint32_t total;
	uint32_t updateid;
	unsigned int rc = cd.Browse(browsetests[i].objectid,
				    browsetests[i].flag,
				    browsetests[i].filter,
				    browsetests[i].start,
				    browsetests[i].requestcount,
				    browsetests[i].sortby,

				    &result, &n, &total, &updateid);
	assert(rc == 0);
	assert(result == browsetests[i].result);
	assert(n      == browsetests[i].n);
	assert(total  == browsetests[i].total);
    }

    for (unsigned int i=0; i<SEARCHTESTS; ++i)
    {
	std::string result;
	uint32_t n;
	uint32_t total;
	uint32_t updateid;
	unsigned int rc = cd.Search(searchtests[i].objectid,
				    searchtests[i].criteria,
				    searchtests[i].filter,
				    searchtests[i].start,
				    searchtests[i].requestcount,
				    searchtests[i].sortby,

				    &result, &n, &total, &updateid);
	assert(rc == 0);
	assert(result == searchtests[i].result);
	assert(n      == searchtests[i].n);
	assert(total  == searchtests[i].total);
    }

    util::Poller poller;
    util::WebServer ws;

    upnpd::MediaServer ms(&mdb, ws.GetPort(), "tests");
    upnp::Server server;
    unsigned int rc = server.Init(&ms);
    assert(rc == 0);

    std::string descurl = (boost::format("http://127.0.0.1:%u/description.xml")
			      % UpnpGetServerPort()).str();

    upnp::Client client;
    rc = client.Init(descurl, ms.UDN());
    assert(rc == 0);

    upnp::ContentDirectory2Client cdc;
    rc = cdc.Init(&client, util::ssdp::s_uuid_contentdirectory);
    assert(rc == 0);

    for (unsigned int i=0; i<BROWSETESTS; ++i)
    {
	std::string result;
	uint32_t n;
	uint32_t total;
	uint32_t updateid;
	rc = cdc.Browse(browsetests[i].objectid,
			browsetests[i].flag,
			browsetests[i].filter,
			browsetests[i].start,
			browsetests[i].requestcount,
			browsetests[i].sortby,
			
			&result, &n, &total, &updateid);
	assert(rc == 0);
	assert(result == browsetests[i].result);
	assert(n      == browsetests[i].n);
	assert(total  == browsetests[i].total);
    }

    for (unsigned int i=0; i<SEARCHTESTS; ++i)
    {
	std::string result;
	uint32_t n;
	uint32_t total;
	uint32_t updateid;
	rc = cdc.Search(searchtests[i].objectid,
			searchtests[i].criteria,
			searchtests[i].filter,
			searchtests[i].start,
			searchtests[i].requestcount,
			searchtests[i].sortby,
			
			&result, &n, &total, &updateid);
	assert(rc == 0);
	assert(result == searchtests[i].result);
	assert(n      == searchtests[i].n);
	assert(total  == searchtests[i].total);
    }

    db::upnpav::Database udb;
    rc = udb.Init(descurl, ms.UDN());
    assert(rc == 0);
    
    db::QueryPtr qp = udb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 0x100));
    db::RecordsetPtr rs = qp->Execute();
    assert(rs);
    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::TITLE) == "mp3");
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);

#endif // HAVE_UPNP

    return 0;
}

#endif // TEST
