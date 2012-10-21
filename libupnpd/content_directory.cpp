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

static unsigned int DIDLFilter(const std::string& filter)
{
    if (filter == "*")
	return upnp::didl::ALL;
    
    unsigned int didl_filter = 0;
    if (strstr(filter.c_str(), "upnp:searchClass"))
	didl_filter |= upnp::didl::SEARCHCLASS;
    if (strstr(filter.c_str(), "upnp:artist"))
	didl_filter |= upnp::didl::ARTIST;
    if (strstr(filter.c_str(), "upnp:album"))
	didl_filter |= upnp::didl::ALBUM;
    if (strstr(filter.c_str(), "upnp:genre"))
	didl_filter |= upnp::didl::GENRE;
    if (strstr(filter.c_str(), "upnp:originalTrackNumber"))
	didl_filter |= upnp::didl::TRACKNUMBER;
    if (strstr(filter.c_str(), "upnp:author"))
	didl_filter |= upnp::didl::AUTHOR;
    if (strstr(filter.c_str(), "dc:date"))
	didl_filter |= upnp::didl::DATE;
    if (strstr(filter.c_str(), "@duration")) // Subparts of RES imply RES
	didl_filter |= upnp::didl::RES | upnp::didl::RES_DURATION;
    if (strstr(filter.c_str(), "@size")) // Subparts of RES imply RES
	didl_filter |= upnp::didl::RES | upnp::didl::RES_SIZE;
    if (strstr(filter.c_str(), "@protocolInfo")) // Subparts of RES imply RES
	didl_filter |= upnp::didl::RES | upnp::didl::RES_PROTOCOL;
    if (strstr(filter.c_str(), "didl-lite:res"))
	didl_filter |= upnp::didl::RES | upnp::didl::RES_DURATION
	    | upnp::didl::RES_SIZE | upnp::didl::RES_PROTOCOL;
    if (strstr(filter.c_str(), "upnp:channelID"))
	didl_filter |= upnp::didl::CHANNELID;
    if (strstr(filter.c_str(), "upnp:storageUsed"))
	didl_filter |= upnp::didl::STORAGEUSED;

    return didl_filter;
}

unsigned int ContentDirectoryImpl::Browse(const std::string& object_id,
					  BrowseFlag browse_flag,
					  const std::string& filter,
					  uint32_t starting_index,
					  uint32_t requested_count,
					  const std::string& /*sort_criteria*/,
					  std::string *result,
					  uint32_t *number_returned,
					  uint32_t *total_matches,
					  uint32_t *update_id)
{
    unsigned int id = (unsigned int)strtoul(object_id.c_str(), NULL, 10);

//    TRACE << "Browse(" << browse_flag << "," << object_id
//	  << "," << starting_index << "," << requested_count
//	  << ")\n";

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
	TRACE << "ID " << object_id << " not found\n";
	return ENOENT;
    }

    std::vector<unsigned int> children;
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &children);

    unsigned int didl_filter = DIDLFilter(filter);

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

    if (browse_flag == BROWSEFLAG_BROWSE_METADATA)
    {
	ss << upnp::didl::FromRecord(m_db, rs, urlprefix.c_str(), didl_filter);
	*number_returned = 1;
	*total_matches = 1;
	*update_id = 1;
    }
    else if (browse_flag == BROWSEFLAG_BROWSE_DIRECT_CHILDREN)
    {
	if (starting_index > children.size())
	{
	    TRACE << "starting_index " << starting_index << " off the end ("
		  << children.size() << ")\n";
	    return ENOENT;
	}

	if (requested_count == 0)
	    requested_count = (uint32_t)children.size();
	
	if (starting_index + requested_count > children.size())
	    requested_count = (uint32_t)(children.size() - starting_index);

	TRACE << "Returning children " << starting_index << ".."
	      << (starting_index+requested_count) << "/"
	      << children.size() << "\n";

	for (unsigned int i = 0; i < requested_count; ++i)
	{
	    qp = m_db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 
				   children[starting_index+i]));
	    rs = qp->Execute();
	    if (rs && !rs->IsEOF())
		ss << upnp::didl::FromRecord(m_db, rs, urlprefix.c_str(),
					     didl_filter);
	    else
	    {
		TRACE << "Child " << children[starting_index+i] << " not found\n";
	    }
	}
	*number_returned = requested_count;
	*total_matches = (uint32_t)children.size();
	*update_id = 1;
    }
    else
    {
	TRACE << "Don't like browse flag '" << browse_flag << "'\n";
	return EINVAL;
    }

    ss << upnp::didl::s_footer;

//    TRACE << "Browse result is " << ss.str() << "\n\n";

    *result = ss.str();
    return 0;
}

unsigned int ContentDirectoryImpl::Search(const std::string& container_id,
					  const std::string& search_criteria,
					  const std::string& filter,
					  uint32_t starting_index,
					  uint32_t requested_count,
					  const std::string& /*sort_criteria*/,
					  std::string *result,
					  uint32_t *number_returned,
					  uint32_t *total_matches,
					  uint32_t *update_id)
{
//    TRACE << "Search(" << search_criteria << ")\n";

    db::QueryPtr qp = m_db->CreateQuery();

    unsigned int collate = 0;

    unsigned int rc = ApplySearchCriteria(qp, search_criteria, &collate);
    if (rc != 0)
	return rc;

    db::RecordsetPtr rs = qp->Execute();

    unsigned int didl_filter = DIDLFilter(filter);

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

    if (requested_count == 0)
	requested_count = (uint32_t)-1;

    while (rs && !rs->IsEOF() && nok < requested_count)
    {
	if (n >= starting_index)
	{
	    if (collate)
	    {
		/** These object IDs never come back to us in Browse, so it's
		 * OK to use fictitious ones.
		 */
		ss << "<container id=\"" << collate << ":" << n
		   << "\" parentID=\"" << container_id << "\" restricted=\"1\""
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
		ss << upnp::didl::FromRecord(m_db, rs, urlprefix.c_str(),
					     didl_filter);
	    }
	    ++nok;
	}

	rs->MoveNext();
	++n;
    }
    ss << upnp::didl::s_footer;

//    TRACE << "Search result is " << ss.str() << " (" << nok << " items)\n";

    *result = ss.str();
    *number_returned = nok;
    *total_matches = (!rs || rs->IsEOF()) ? n : 0;
    *update_id = 1;

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

unsigned int ContentDirectoryImpl::GetSystemUpdateID(uint32_t *pui)
{
    *pui = 1;
    return 0;
}

unsigned int ContentDirectoryImpl::GetFeatureList(std::string *fl)
{
    *fl = 
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<Features xmlns=\"urn:schemas-upnp-org:av:avs\">"
	" <Feature name=\"TUNER\" version=\"1\">"
	"  <objectIDs>240</objectIDs>"
	" </Feature>"
	"</Features>";
    
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
    upnp::ContentDirectory2::BrowseFlag flag;
    const char *filter;
    unsigned int start;
    unsigned int requestcount;
    const char *sortby;

    const char *result;
    unsigned int n;
    unsigned int total;
} browsetests[] = {
    { "0", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "*", 0, 0, "",

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

    { "0", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_DIRECT_CHILDREN, "*", 0, 0, "",

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

      "</DIDL-Lite>", 5, 5 },

    // Browses with filters
    { "291", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_DIRECT_CHILDREN, "upnp:genre,dc:date", 0, 2, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"

      "<item id=\"302\" parentID=\"289\" restricted=\"1\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>1996-01-01</dc:date>"
      "</item>"
      "<item id=\"297\" parentID=\"0\" restricted=\"1\">"
      "<dc:title>Bigmouth</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>1992-01-01</dc:date>"
      "</item>"
      "</DIDL-Lite>", 2, 4 },

    { "0", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"1\" childCount=\"5\""
      " searchable=\"1\">"
      "<dc:title>mp3</dc:title>"
      "<upnp:class>object.container.storageFolder</upnp:class>"
      "</container></DIDL-Lite>",
      1, 1 },

    { "0", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "upnp:searchClass", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"1\" childCount=\"5\""
      " searchable=\"1\">"
      "<dc:title>mp3</dc:title>"
      "<upnp:class>object.container.storageFolder</upnp:class>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum</upnp:searchClass>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.genre.musicGenre</upnp:searchClass>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.person.musicArtist</upnp:searchClass>"
      "</container></DIDL-Lite>",
      1, 1 },

    { "0", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "upnp:storageUsed", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"1\" childCount=\"5\""
      " searchable=\"1\">"
      "<dc:title>mp3</dc:title>"
      "<upnp:class>object.container.storageFolder</upnp:class>"
      "<upnp:storageUsed>-1</upnp:storageUsed>"
      "</container></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"1\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"1\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Underworld</upnp:artist>"
      "<upnp:album>1992-2002</upnp:album>"
      "<upnp:originalTrackNumber>9</upnp:originalTrackNumber>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>1996-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"10296067\""
      " duration=\"0:07:34.00\">12e</res>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "upnp:artist,upnp:album", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"1\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Underworld</upnp:artist>"
      "<upnp:album>1992-2002</upnp:album>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "upnp:genre,upnp:originalTrackNumber", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"1\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:originalTrackNumber>9</upnp:originalTrackNumber>"
      "<upnp:genre>Electronica</upnp:genre>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory2::BROWSEFLAG_BROWSE_METADATA, "dc:date,didl-lite:res@size", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"1\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<dc:date>1996-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"10296067\""
      " duration=\"0:07:34.00\">12e</res>"
      "</item></DIDL-Lite>",
      1, 1 },
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

    // Album query
    { "0",
      "upnp:class = \"object.container.album.musicAlbum\"",
      "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"3:0\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>1992-2002</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>1992-2002</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass>"
      "</container>"
      "<container id=\"3:1\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>Everything Is</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>Everything Is</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass>"
      "</container>"
      "<container id=\"3:2\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>The Garden</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>The Garden</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass>"
      "</container>"
      "<container id=\"3:3\" parentID=\"0\" restricted=\"1\" searchable=\"1\">"
      "<dc:title>Violator</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>Violator</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass>"
      "</container>"
      "</DIDL-Lite>", 4, 4 },

    // Genre query
    { "0",
      "upnp:class = \"object.container.genre.musicGenre\"",
      "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"5:0\" parentID=\"0\" restricted=\"1\" searchable=\"1\"><dc:title>Ambient</dc:title><upnp:class>object.container.genre.musicGenre</upnp:class><upnp:genre>Ambient</upnp:genre><upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass></container><container id=\"5:1\" parentID=\"0\" restricted=\"1\" searchable=\"1\"><dc:title>Electronica</dc:title><upnp:class>object.container.genre.musicGenre</upnp:class><upnp:genre>Electronica</upnp:genre><upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass></container><container id=\"5:2\" parentID=\"0\" restricted=\"1\" searchable=\"1\"><dc:title>Rock</dc:title><upnp:class>object.container.genre.musicGenre</upnp:class><upnp:genre>Rock</upnp:genre><upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass></container>"
      "</DIDL-Lite>", 3, 3 },
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
	if (result != browsetests[i].result)
	{
	    TRACE << "Got:\n" << result << "\nexpected:\n"
		  << browsetests[i].result << "\nFAIL.\n";
	}
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
	if (result != searchtests[i].result)
	{
	    TRACE << "Got:\n" << result << "\nexpected:\n"
		  << searchtests[i].result << "\nFAIL.\n";
	}
	assert(result == searchtests[i].result);
	assert(n      == searchtests[i].n);
	assert(total  == searchtests[i].total);
    }

    std::string caps;
    unsigned int rc = cd.GetSearchCapabilities(&caps);
    assert(rc == 0);
    assert(caps == "*");
    rc = cd.GetSortCapabilities(&caps);
    assert(rc == 0);
    assert(caps == "" );
    rc = cd.GetFeatureList(&caps);
    assert(caps == 
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<Features xmlns=\"urn:schemas-upnp-org:av:avs\">"
	" <Feature name=\"TUNER\" version=\"1\">"
	"  <objectIDs>240</objectIDs>"
	" </Feature>"
	   "</Features>");

    // Now again via UPnP

    util::Poller poller;
    util::WebServer ws;

    upnpd::MediaServer ms(&mdb, ws.GetPort(), "tests");
    upnp::Server server;
    rc = server.Init(&ms, ws.GetPort());
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

    rc = cdc.GetSearchCapabilities(&caps);
    assert(rc == 0);
    assert(caps == "*");
    rc = cdc.GetSortCapabilities(&caps);
    assert(rc == 0);
    assert(caps == "" );
    rc = cdc.GetFeatureList(&caps);
    assert(caps == 
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<Features xmlns=\"urn:schemas-upnp-org:av:avs\">"
	" <Feature name=\"TUNER\" version=\"1\">"
	"  <objectIDs>240</objectIDs>"
	" </Feature>"
	   "</Features>");

    // Now via a dbupnp

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

#endif // HAVE_UPNP

    return 0;
}

#endif // TEST
