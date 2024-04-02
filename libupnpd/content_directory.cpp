#include "config.h"
#include "content_directory.h"
#include "libmediadb/schema.h"
#include "libmediadb/didl.h"
#include "libmediadb/db.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libutil/counted_pointer.h"
#include "libutil/trace.h"
#include "libutil/xmlescape.h"
#include "libutil/socket.h"
#include "libutil/printf.h"
#include "libutil/bind.h"
#include "libupnp/soap_info_source.h"
#include "search.h"
#include <sstream>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

LOG_DECL(CDS);

namespace upnpd {

template <typename RET, typename ...ARGS>
class Callback
{
    typedef RET (*pfn)(void*, ARGS...);

    void *m_ptr;
    pfn m_pfn;

public:
    Callback() :  m_ptr(NULL), m_pfn(NULL) {}
    Callback(void *ptr, pfn ppfn) : m_ptr(ptr), m_pfn(ppfn) {}

    RET operator()(ARGS... args) const { return (*m_pfn)(m_ptr, args...); }
};

template <typename T>
class Binder;

template <typename RET, typename T, typename ...ARGS>
class Binder<RET (T::*)(ARGS...)>
{
    template <RET (T::*FN)(ARGS...)>
    static RET Call(void *ptr, ARGS... args)
    {
        return (((T*)ptr)->*FN)(args...);
    }

public:
    template <RET (T::*FN)(ARGS...)>
    static Callback<RET,ARGS...> bind(T *t)
    {
        return Callback<RET,ARGS...>((void*)t, &Call<FN>);
    }
};

#define BIND(x,y) Binder<decltype(x)>::bind<x>(y)

template <typename T>
struct IntFuncs
{
    template <T N>
    static T IntFunc()
    {
        return N;
    }
};

unsigned answer = IntFuncs<unsigned>::IntFunc<42>();

typedef Callback<unsigned int, unsigned int, unsigned int, const std::string&> BDC;

class CDA
{
public:
    unsigned browse(uint32_t starting_index, BDC callback)
    {
        return callback(2, starting_index+1, "hello");
    }
};

class Foo
{
    unsigned onBrowseDone(unsigned int, unsigned int, const std::string&);

public:
    void fnord(CDA *cda)
    {
        cda->browse(3, BIND(&Foo::onBrowseDone, this));
    }
};

ContentDirectoryImpl::ContentDirectoryImpl(mediadb::Database *db,
					   upnp::soap::InfoSource *info_source)
    : m_db(db),
      m_info_source(info_source)
{
}

static unsigned int DIDLFilter(const std::string& filter)
{
    if (filter == "*")
	return mediadb::didl::ALL;
    
    unsigned int didl_filter = 0;
    if (strstr(filter.c_str(), "upnp:searchClass"))
	didl_filter |= mediadb::didl::SEARCHCLASS;
    if (strstr(filter.c_str(), "upnp:artist"))
	didl_filter |= mediadb::didl::ARTIST;
    if (strstr(filter.c_str(), "upnp:album"))
	didl_filter |= mediadb::didl::ALBUM;
    if (strstr(filter.c_str(), "upnp:genre"))
	didl_filter |= mediadb::didl::GENRE;
    if (strstr(filter.c_str(), "upnp:originalTrackNumber"))
	didl_filter |= mediadb::didl::TRACKNUMBER;
    if (strstr(filter.c_str(), "upnp:author"))
	didl_filter |= mediadb::didl::AUTHOR;
    if (strstr(filter.c_str(), "dc:date"))
	didl_filter |= mediadb::didl::DATE;
    if (strstr(filter.c_str(), "@duration")) // Subparts of RES imply RES
	didl_filter |= mediadb::didl::RES | mediadb::didl::RES_DURATION;
    if (strstr(filter.c_str(), "@size")) // Subparts of RES imply RES
	didl_filter |= mediadb::didl::RES | mediadb::didl::RES_SIZE;
    if (strstr(filter.c_str(), "@protocolInfo")) // Subparts of RES imply RES
	didl_filter |= mediadb::didl::RES | mediadb::didl::RES_PROTOCOL;
    if (strstr(filter.c_str(), "didl-lite:res"))
	didl_filter |= mediadb::didl::RES | mediadb::didl::RES_DURATION
	    | mediadb::didl::RES_SIZE | mediadb::didl::RES_PROTOCOL;
    if (strstr(filter.c_str(), "upnp:channelID"))
	didl_filter |= mediadb::didl::CHANNELID;
    if (strstr(filter.c_str(), "upnp:storageUsed"))
	didl_filter |= mediadb::didl::STORAGEUSED;

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
    
    LOG(CDS) << "Browse(" << browse_flag << "," << object_id
	     << "," << starting_index << "," << requested_count
	     << ")\n";

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
    ss << mediadb::didl::s_header;

    std::string urlprefix;
    if (m_info_source)
    {
	util::IPEndPoint ipe = m_info_source->GetCurrentEndPoint();
	urlprefix = "http://" + ipe.ToString() + "/content/";
    }

    if (browse_flag == BROWSEFLAG_BROWSE_METADATA)
    {
	ss << mediadb::didl::FromRecord(m_db, rs, urlprefix.c_str(), 
					didl_filter);
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

	LOG(CDS) << "Returning children " << starting_index << ".."
		 << (starting_index+requested_count) << "/"
		 << children.size() << "\n";

	for (unsigned int i = 0; i < requested_count; ++i)
	{
	    qp = m_db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 
				   children[starting_index+i]));
	    rs = qp->Execute();
	    if (rs && !rs->IsEOF())
		ss << mediadb::didl::FromRecord(m_db, rs, urlprefix.c_str(),
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

    ss << mediadb::didl::s_footer;

    LOG(CDS) << "Browse result is " << ss.str() << "\n\n";

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
    LOG(CDS) << "Search(" << search_criteria
	     << "," << starting_index << "," << requested_count << ")\n";

    db::QueryPtr qp = m_db->CreateQuery();

    unsigned int collate = 0;

    unsigned int rc = ApplySearchCriteria(qp.get(), search_criteria, &collate);
    if (rc != 0)
	return rc;

    db::RecordsetPtr rs = qp->Execute();

    unsigned int didl_filter = DIDLFilter(filter);

    std::ostringstream ss;
    ss << mediadb::didl::s_header;

    std::string urlprefix;
    if (m_info_source)
    {
	util::IPEndPoint ipe = m_info_source->GetCurrentEndPoint();
	urlprefix = "http://" + ipe.ToString() + "/content/";
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
		   << "\" parentID=\"" << container_id << "\" restricted=\"true\""
		    " searchable=\"true\">"
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
		ss << mediadb::didl::FromRecord(m_db, rs, urlprefix.c_str(),
					     didl_filter);
	    }
	    ++nok;
	}

	rs->MoveNext();
	++n;
    }
    ss << mediadb::didl::s_footer;

    LOG(CDS) << "Returned " << n << " now scanning for more\n";

    while (rs && !rs->IsEOF())
    {
	rs->MoveNext();
	++n;
    }

    LOG(CDS) << "Search result is " << ss.str() << " (" << nok << "/" << n << " items)\n";

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
    *fl = util::Printf() <<
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<Features xmlns=\"urn:schemas-upnp-org:av:avs\">"
	" <Feature name=\"TUNER\" version=\"1\">"
	"  <objectIDs>"
			 << (unsigned)mediadb::RADIO_ROOT << ","
			 << (unsigned)mediadb::TV_ROOT
			 << "</objectIDs>"
	" </Feature>"
	" <Feature name=\"EPG\" version=\"1\">"
	"  <objectIDs>" << (unsigned)mediadb::EPG_ROOT << "</objectIDs>"
	" </Feature>"
	"</Features>";
    
    return 0;
}

unsigned int ContentDirectoryImpl::GetServiceResetToken(std::string *srt)
{
    *srt = "0";
    return 0;
}

} // namespace upnpd


        /* Unit tests */


#ifdef TEST

# include "libutil/scheduler.h"
# include "libutil/http_client.h"
# include "libutil/http_server.h"
# include "libutil/worker_thread_pool.h"
# include "libdblocal/db.h"
# include "libdbsteam/db.h"
# include "libdbupnp/database.h"
# include "libupnp/ssdp.h"
# include "libupnp/server.h"
# include "libupnp/ContentDirectory_client.h"
# include "libmediadb/xml.h"
# include "media_server.h"
# include <boost/format.hpp>
#if HAVE_WINDOWS_H
# include <windows.h>
#endif

static const struct {
    const char *objectid;
    upnp::ContentDirectory::BrowseFlag flag;
    const char *filter;
    unsigned int start;
    unsigned int requestcount;
    const char *sortby;

    const char *result;
    unsigned int n;
    unsigned int total;
} browsetests[] = {
    { "0", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"true\" childCount=\"5\""
      " searchable=\"true\">"
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

    { "0", upnp::ContentDirectory::BROWSEFLAG_BROWSE_DIRECT_CHILDREN, "*", 0, 0, "",

"<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
" xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"

"<container id=\"289\" parentID=\"0\" restricted=\"true\" childCount=\"17\">"
"<dc:title>1992-2002</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"290\" parentID=\"0\" restricted=\"true\" childCount=\"12\">"
"<dc:title>Everything Is</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"291\" parentID=\"0\" restricted=\"true\" childCount=\"4\">"
"<dc:title>SongsStartingWithB</dc:title>"
"<upnp:class>object.container.playlistContainer</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"293\" parentID=\"0\" restricted=\"true\" childCount=\"13\">"
"<dc:title>The Garden</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

"<container id=\"294\" parentID=\"0\" restricted=\"true\" childCount=\"10\">"
"<dc:title>Violator</dc:title>"
"<upnp:class>object.container.storageFolder</upnp:class>"
"<upnp:storageUsed>-1</upnp:storageUsed>"
"</container>"

      "</DIDL-Lite>", 5, 5 },

    // Browses with filters
    { "291", upnp::ContentDirectory::BROWSEFLAG_BROWSE_DIRECT_CHILDREN, "upnp:genre,dc:date", 0, 2, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"

      "<item id=\"302\" parentID=\"289\" restricted=\"true\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>1996-01-01</dc:date>"
      "</item>"
      "<item id=\"297\" parentID=\"0\" restricted=\"true\">"
      "<dc:title>Bigmouth</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>1992-01-01</dc:date>"
      "</item>"
      "</DIDL-Lite>", 2, 4 },

    { "0", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"true\" childCount=\"5\""
      " searchable=\"true\">"
      "<dc:title>mp3</dc:title>"
      "<upnp:class>object.container.storageFolder</upnp:class>"
      "</container></DIDL-Lite>",
      1, 1 },

    { "0", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "upnp:searchClass", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"true\" childCount=\"5\""
      " searchable=\"true\">"
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

    { "0", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "upnp:storageUsed", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"0\" parentID=\"-1\" restricted=\"true\" childCount=\"5\""
      " searchable=\"true\">"
      "<dc:title>mp3</dc:title>"
      "<upnp:class>object.container.storageFolder</upnp:class>"
      "<upnp:storageUsed>-1</upnp:storageUsed>"
      "</container></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"true\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"true\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Underworld</upnp:artist>"
      "<upnp:album>1992-2002</upnp:album>"
      "<upnp:originalTrackNumber>9</upnp:originalTrackNumber>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>1996-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"10296067\""
      " duration=\"0:07:34.00\">12e?_range=1</res>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "upnp:artist,upnp:album", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"true\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Underworld</upnp:artist>"
      "<upnp:album>1992-2002</upnp:album>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "upnp:genre,upnp:originalTrackNumber", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"true\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:originalTrackNumber>9</upnp:originalTrackNumber>"
      "<upnp:genre>Electronica</upnp:genre>"
      "</item></DIDL-Lite>",
      1, 1 },

    { "302", upnp::ContentDirectory::BROWSEFLAG_BROWSE_METADATA, "dc:date,didl-lite:res@size", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<item id=\"302\" parentID=\"289\" restricted=\"true\">"
      "<dc:title>Born Slippy Nuxx</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<dc:date>1996-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"10296067\""
      " duration=\"0:07:34.00\">12e?_range=1</res>"
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

      "<container id=\"291\" parentID=\"0\" restricted=\"true\" childCount=\"4\">"
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
      "<container id=\"291\" parentID=\"0\" restricted=\"true\" childCount=\"4\">"
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
      "<item id=\"353\" parentID=\"290\" restricted=\"true\">"
      "<dc:title>Headlights</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Nine Black Alps</upnp:artist>"
      "<upnp:album>Everything Is</upnp:album>"
      "<upnp:originalTrackNumber>5</upnp:originalTrackNumber>"
      "<upnp:genre>Electronica</upnp:genre>"
      "<dc:date>2005-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"4209148\""
      " duration=\"0:02:44.00\">161?_range=1</res>"
      "<res protocolInfo=\"http-get:*:audio/x-flac:*\" size=\"22115187\""
      " duration=\"0:02:44.00\">162?_range=1</res>"
      "</item>"
      "</DIDL-Lite>", 1, 1 },

    // This is the Soundbridge artists menu query
    { "0",
      "upnp:class = \"object.container.person.musicArtist\""
      " and @refID exists false", "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"2:0\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
      "<dc:title>Depeche Mode</dc:title>"
      "<upnp:class>object.container.person.musicArtist</upnp:class>"
      "<upnp:artist>Depeche Mode</upnp:artist>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum"
      "</upnp:searchClass>"
      "</container>"
      "<container id=\"2:1\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
      "<dc:title>Nine Black Alps</dc:title>"
      "<upnp:class>object.container.person.musicArtist</upnp:class>"
      "<upnp:artist>Nine Black Alps</upnp:artist>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum"
      "</upnp:searchClass>"
      "</container>"
      "<container id=\"2:2\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
      "<dc:title>Underworld</dc:title>"
      "<upnp:class>object.container.person.musicArtist</upnp:class>"
      "<upnp:artist>Underworld</upnp:artist>"
      "<upnp:searchClass includeDerived=\"0\">"
      "object.container.album.musicAlbum"
      "</upnp:searchClass>"
      "</container>"
      "<container id=\"2:3\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
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
      "<container id=\"3:0\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
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
      "<item id=\"331\" parentID=\"0\" restricted=\"true\">"
      "<dc:title>Blue Dress</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Depeche Mode</upnp:artist>"
      "<upnp:album>Violator</upnp:album>"
      "<upnp:originalTrackNumber>8</upnp:originalTrackNumber>"
      "<upnp:genre>Rock</upnp:genre>"
      "<dc:date>1990-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"9557588\""
      " duration=\"0:05:38.00\">14b?_range=1</res>"
      "<res protocolInfo=\"http-get:*:audio/x-flac:*\" size=\"33977715\""
      " duration=\"0:05:38.00\">15f?_range=1</res>"
      "</item>"
      "<item id=\"366\" parentID=\"0\" restricted=\"true\">"
      "<dc:title>World In My Eyes</dc:title>"
      "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
      "<upnp:artist>Depeche Mode</upnp:artist>"
      "<upnp:album>Violator</upnp:album>"
      "<upnp:originalTrackNumber>1</upnp:originalTrackNumber>"
      "<upnp:genre>Rock</upnp:genre>"
      "<dc:date>1990-01-01</dc:date>"
      "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"7629896\""
      " duration=\"0:04:27.00\">"
      "16e?_range=1"
      "</res>"
      "<res protocolInfo=\"http-get:*:audio/x-flac:*\" size=\"30686347\""
      " duration=\"0:04:27.00\">"
      "16f?_range=1"
      "</res>"
      "</item>"
      "</DIDL-Lite>", 2, 9 },

    // Album query
    { "0",
      "upnp:class = \"object.container.album.musicAlbum\"",
      "*", 0, 0, "",

      "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
      " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
      " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
      "<container id=\"3:0\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
      "<dc:title>1992-2002</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>1992-2002</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass>"
      "</container>"
      "<container id=\"3:1\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
      "<dc:title>Everything Is</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>Everything Is</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass>"
      "</container>"
      "<container id=\"3:2\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
      "<dc:title>The Garden</dc:title>"
      "<upnp:class>object.container.album.musicAlbum</upnp:class>"
      "<upnp:album>The Garden</upnp:album>"
      "<upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass>"
      "</container>"
      "<container id=\"3:3\" parentID=\"0\" restricted=\"true\" searchable=\"true\">"
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
      "<container id=\"5:0\" parentID=\"0\" restricted=\"true\" searchable=\"true\"><dc:title>Ambient</dc:title><upnp:class>object.container.genre.musicGenre</upnp:class><upnp:genre>Ambient</upnp:genre><upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass></container><container id=\"5:1\" parentID=\"0\" restricted=\"true\" searchable=\"true\"><dc:title>Electronica</dc:title><upnp:class>object.container.genre.musicGenre</upnp:class><upnp:genre>Electronica</upnp:genre><upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass></container><container id=\"5:2\" parentID=\"0\" restricted=\"true\" searchable=\"true\"><dc:title>Rock</dc:title><upnp:class>object.container.genre.musicGenre</upnp:class><upnp:genre>Rock</upnp:genre><upnp:searchClass includeDerived=\"0\">object.item.audioItem.musicTrack</upnp:searchClass></container>"
      "</DIDL-Lite>", 3, 3 },
};

enum { SEARCHTESTS = sizeof(searchtests)/sizeof(searchtests[0]) };


int main(int, char**)
{
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

    upnpd::ContentDirectoryImpl cd(&mdb, NULL);

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
	if (result != searchtests[i].result
            || total != searchtests[i].total)
	{
	    TRACE << "Search test " << i << ": " << searchtests[i].criteria
		  << "(" << searchtests[i].total << ")" << "\n";
	    TRACE << "Got:\n" << result << "\nexpected:\n"
		  << searchtests[i].result
                  << "(" << total << ")"
                  << "\nFAIL.\n";
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
	"  <objectIDs>240,208</objectIDs>"
	" </Feature>"
	" <Feature name=\"EPG\" version=\"1\">"
	"  <objectIDs>224</objectIDs>"
	" </Feature>"
	"</Features>");

    // Now again via UPnP

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);
    util::BackgroundScheduler poller;
    util::http::Server ws(&poller, &wtp);
    upnp::ssdp::Responder ssdp(&poller, NULL);

    ws.Init();
    wtp.PushTask(util::SchedulerTask::Create(&poller));

    upnp::Server server(&poller, &wc, &ws, &ssdp);
    upnpd::MediaServer ms(&mdb, NULL);
    rc = ms.Init(&server, "tests");
    assert(rc == 0);
    rc = server.Init();
    assert(rc == 0);

    std::string descurl = (boost::format("http://127.0.0.1:%u/upnp/description.xml")
			      % ws.GetPort()).str();

    upnp::DeviceClient client(&wc, &ws, &poller);
    rc = client.Init(descurl, ms.GetUDN());
    assert(rc == 0);

    upnp::ContentDirectoryClient cdc(&client,
				     upnp::s_service_id_content_directory);
    rc = cdc.Init();
    assert(rc == 0);
    
#ifdef WIN32
    Sleep(2000);
#else
    sleep(2);
#endif

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
	if (result != browsetests[i].result)
	{
	    TRACE << "*** Test failing\n" << result << "*** ISN'T\n"
		  << browsetests[i].result << "***\n";
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
    TRACE << "caps='" << caps << "'\n";
    assert(caps ==
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<Features xmlns=\"urn:schemas-upnp-org:av:avs\">"
	" <Feature name=\"TUNER\" version=\"1\">"
	"  <objectIDs>240,208</objectIDs>"
	" </Feature>"
	" <Feature name=\"EPG\" version=\"1\">"
	"  <objectIDs>224</objectIDs>"
	" </Feature>"
	"</Features>");

    // Now via a dbupnp

    db::upnp::Database udb(&wc, &ws, &poller);
    rc = udb.Init(descurl, ms.GetUDN());
    assert(rc == 0);
    
#ifdef WIN32
    Sleep(2000);
#else
    sleep(2);
#endif

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

//    TRACE << "Exiting\n";

    return 0;
}

#endif // TEST
