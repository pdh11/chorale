#include "config.h"
#include "didl.h"
#include "libutil/trace.h"
#include "db.h"
#include "schema.h"
#include "libutil/xmlescape.h"
#include "libutil/xml.h"
#include "libutil/string_stream.h"
#include <boost/format.hpp>
#include <sstream>

namespace mediadb {
namespace didl {

class ParserObserver: public xml::SaxParserObserver
{
    MetadataList *m_results;
    Metadata m_current_item;
    Field m_current_field;
    bool m_in_item;

public:
    ParserObserver(MetadataList *ml) : m_results(ml), m_in_item(false) {}

    unsigned int OnBegin(const char *tag)
    {
	if (!strcmp(tag, "item") || !strcmp(tag, "container"))
	{
	    m_current_item.clear();
	    m_current_field.tag.clear();
	    m_in_item = true;
	}
	else
	{
	    if (m_in_item)
		m_current_field.tag = tag;
	}
	return 0;
    }

    unsigned int OnEnd(const char *tag)
    {
	if (!strcmp(tag, "item") || !strcmp(tag, "container"))
	{
	    m_results->push_back(m_current_item);
	    m_in_item = false;
	}
	else
	{
	    if (m_in_item)
	    {
		m_current_item.push_back(m_current_field);
		m_current_field.content.clear();
		m_current_field.tag.clear();
		m_current_field.attributes.clear();
	    }
	}
	return 0; 
    }

    unsigned int OnAttribute(const char *name, const char* value)
    {
	if (m_current_field.tag.empty())
	{
	    // ID is really an attribute of the item tag, but it's
	    // treated specially
	    if (!strcmp(name, "id"))
	    {
		m_current_field.tag = name;
		m_current_field.content = value;
		m_current_item.push_back(m_current_field);
		m_current_field.tag.clear();
		m_current_field.content.clear();
	    }
	}
	else
	    m_current_field.attributes[name] = value;
	return 0; 
    }

    unsigned int OnContent(const char *content)
    {
	m_current_field.content += content;
	return 0;
    }
};

MetadataList Parse(const std::string& xml)
{
    MetadataList results;
    ParserObserver po(&results);
    xml::SaxParser parser(&po);
    parser.Parse(util::StringStream::Create(xml));
    return results;
}


unsigned int ToRecord(const Metadata& md, db::RecordsetPtr rs)
{
    for (Metadata::const_iterator i = md.begin();
	 i != md.end();
	 ++i)
    {
	if (i->tag == "dc:title")
	    rs->SetString(mediadb::TITLE, i->content);
	else if (i->tag == "upnp:artist")
	    rs->SetString(mediadb::ARTIST, i->content);
	else if (i->tag == "upnp:album")
	    rs->SetString(mediadb::ALBUM, i->content);
	else if (i->tag == "upnp:genre")
	    rs->SetString(mediadb::GENRE, i->content);
	else if (i->tag == "upnp:originalTrackNumber")
	{
	    unsigned int tracknr
		= (unsigned int)strtoul(i->content.c_str(), NULL, 10);
	    rs->SetInteger(mediadb::TRACKNUMBER, tracknr);
	}
	else if (i->tag == "upnp:class")
	{
	    unsigned int type = mediadb::FILE;
	    
	    if (i->content == "object.container.storageFolder"
		|| i->content == "object.container")
		type = mediadb::DIR;
	    else if (i->content == "object.item.audioItem.musicTrack")
		type = mediadb::TUNE;
	    else if (i->content == "object.container.playlistContainer")
		type = mediadb::PLAYLIST;
	    else if (!strncmp(i->content.c_str(), "object.container.", 17))
		type = mediadb::DIR;
	    else
		TRACE << "Unknown class '" << i->content << "'\n";
		
	    rs->SetInteger(mediadb::TYPE, type);
	}
	else if (i->tag == "res")
	{
	    Attributes::const_iterator ci =
		i->attributes.find("protocolInfo");
	    if (ci != i->attributes.end()
		&& ci->second == "http-get:*:audio/mpeg:*")
	    {
		rs->SetInteger(mediadb::CODEC, mediadb::MP3);
		rs->SetString(mediadb::PATH, i->content);
		
		ci = i->attributes.find("duration");
		if (ci != i->attributes.end())
		{
		    unsigned int h=0, m=0, s=0, f=0;
		    if (sscanf(ci->second.c_str(), "%u:%u:%u.%u", 
			       &h, &m, &s, &f) == 4
			|| sscanf(ci->second.c_str(), "%u:%u:%u",
				  &h, &m, &s) == 3
			|| sscanf(ci->second.c_str(), "%u:%u.%u",
				  &m, &s, &f) == 3
			|| sscanf(ci->second.c_str(), "%u:%u",
				  &m, &s) == 2)
		    {
			unsigned int ms
			    = h*3600*1000 + m*60*1000 + s*1000 + f * 10;
			rs->SetInteger(mediadb::DURATIONMS, ms);
		    }
		}
	    }
	    else
	    {
		TRACE << "Don't like protocol:\n" << i->attributes;
	    }
	}
//	    else
//		TRACE << "Unknown tag '" << i->tag << "'\n";
    }
    return 0;
}

static std::string IfPresent(const char *tag, const std::string& value)
{
    if (value.empty())
	return std::string();
    return (boost::format("<%s>%u</%s>") % tag % util::XmlEscape(value) % tag).str();
}

static std::string IfPresent2(const char *tag, const char *attr,
			      const std::string& value)
{
    if (value.empty())
	return std::string();
    return (boost::format("<%s %s>%u</%s>") % tag % attr % util::XmlEscape(value) % tag).str();
}

static std::string IfPresent(const char *tag, unsigned int value)
{
    if (!value)
	return std::string();
    return (boost::format("<%s>%u</%s>") % tag % value % tag).str();
}

static std::string ResItem(mediadb::Database *db, db::RecordsetPtr rs,
			   const char *urlprefix, unsigned int filter)
{
    const char *mimetype = "audio/mpeg"; // Until proven otherwise

    /* RFC3003 says that MP2 should be audio/mpeg, just like MP3. But that's a
     * bit unhelpful in the case where the client can play MP3 but not MP2.
     * Perhaps we should use audio/x-mp2 instead.
     */

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    unsigned int codec = rs->GetInteger(mediadb::CODEC);
    switch (type)
    {
    case mediadb::TUNE:
    case mediadb::RADIO:
    case mediadb::TUNEHIGH:
    case mediadb::SPOKEN:
	switch (codec)
	{
	case mediadb::FLAC: mimetype = "audio/x-flac"; break;
	case mediadb::OGGVORBIS: mimetype = "application/ogg"; break;
	case mediadb::PCM: mimetype = "audio/L16"; break;
	}
	break;
    case mediadb::IMAGE:
	mimetype = "image/jpeg";
	break;
    case mediadb::VIDEO:
	mimetype = "video/mpeg";
	break;
    default:
	break;
    }
    unsigned int id = rs->GetInteger(mediadb::ID);
    std::string url = db->GetURL(id);
    if (strncmp(url.c_str(), "http:", 5) && urlprefix)
	url = (boost::format("%s%x") % urlprefix % id).str();

    url = util::XmlEscape(url);

    std::string result = "<res";
    if (filter & RES_PROTOCOL)
    {
	result += " protocolInfo=\"http-get:*:";
	result += mimetype;
	result += ":*\"";
    }
    if (filter & RES_SIZE)
    {
	result += (boost::format(" size=\"%u\"")
		   % rs->GetInteger(mediadb::SIZEBYTES)).str();
    }
    if (filter & RES_DURATION)
    {
	unsigned int durationms = rs->GetInteger(mediadb::DURATIONMS);
	result += (boost::format(" duration=\"%u:%02u:%02u.00\"")
		   % (durationms/3600000)
		   % ((durationms/ 60000) % 60)
		   % ((durationms/  1000) % 60)).str();
    }
    return result + ">" + url + "</res>";
}

const char s_header[] =
    "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
    " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
    " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">";
const char s_footer[] = "</DIDL-Lite>";


/* FromRecord */


std::string FromRecord(mediadb::Database *db, db::RecordsetPtr rs,
		       const char *urlprefix, unsigned int filter)
{
    std::ostringstream os;
    unsigned int id = rs->GetInteger(mediadb::ID);
    int parentid = (int)rs->GetInteger(mediadb::IDPARENT);
    if (id == 0x100)
    {
	id = 0;
	parentid = -1;
    }
    if (parentid == 0x100)
	parentid = 0;

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    if (type == mediadb::DIR || type == mediadb::PLAYLIST)
    {
	std::vector<unsigned int> children;
	mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &children);
	os << "<container id=\"" << id << "\" parentID=\"" << parentid
	   << "\" restricted=\"1\" childCount=\"" << children.size() << "\"";

	/** IDs <= 0x100 are "magic" root ids: browse root, radio root, epg
	 * root. Unlike everything else, they're searchable.
	 */
	if (id <= 0x100)
	    os << " searchable=\"1\"";
	
	os << ">"
	    "<dc:title>" << util::XmlEscape(rs->GetString(mediadb::TITLE))
	   << "</dc:title>"
	    "<upnp:class>";

	if (type == mediadb::DIR)
	{
	    if (id == mediadb::RADIO_ROOT)
		os << "object.container.channelGroup";
	    else
		os << "object.container.storageFolder";
	}
	else
	    os << "object.container.playlistContainer";

	os << "</upnp:class>";

	if (filter & STORAGEUSED)
	    os << "<upnp:storageUsed>-1</upnp:storageUsed>";

	if (id == 0 && (filter & SEARCHCLASS))
	{
	    os << "<upnp:searchClass includeDerived=\"0\">"
		"object.container.album.musicAlbum"
		"</upnp:searchClass>"
		"<upnp:searchClass includeDerived=\"0\">"
		"object.container.genre.musicGenre"
		"</upnp:searchClass>"
		"<upnp:searchClass includeDerived=\"0\">"
		"object.container.person.musicArtist"
		"</upnp:searchClass>";
	}

	os << "</container>";
    }
    else
    {
	const char *upnpclass = "object.item.audioItem.musicTrack";
	switch (type)
	{
	case mediadb::RADIO:
	    upnpclass = "object.item.audioItem.audioBroadcast";
	    break;
	case mediadb::IMAGE:
	    upnpclass = "object.item.imageItem.photo";
	    break;
	}

	os << "<item id=\"" << id << "\" parentID=\"" << parentid
	   << "\" restricted=\"1\">"

	    "<dc:title>" << util::XmlEscape(rs->GetString(mediadb::TITLE)) <<
	    "</dc:title>"
	    "<upnp:class>" << upnpclass << "</upnp:class>";

	if (filter & ARTIST)
	    os << IfPresent("upnp:artist", rs->GetString(mediadb::ARTIST));
	if (filter & ALBUM)
	    os << IfPresent("upnp:album", rs->GetString(mediadb::ALBUM));
	if (filter & TRACKNUMBER)
	    os << IfPresent("upnp:originalTrackNumber",
			    rs->GetInteger(mediadb::TRACKNUMBER));
	if (filter & GENRE)
	    os << IfPresent("upnp:genre", rs->GetString(mediadb::GENRE));
	if (filter & DATE)
	{
	    unsigned int y = rs->GetInteger(mediadb::YEAR);
	    if (y)
		os << "<dc:date>" << y << "-01-01</dc:date>";
	}

	if (filter & AUTHOR)
	{
	    os << IfPresent2("upnp:author", "role=\"Composer\"", 
			     rs->GetString(mediadb::COMPOSER));
	    os << IfPresent2("upnp:author", "role=\"Remix\"", 
			     rs->GetString(mediadb::REMIXED));
	    os << IfPresent2("upnp:author", "role=\"Ensemble\"", 
			     rs->GetString(mediadb::ENSEMBLE));
	    os << IfPresent2("upnp:author", "role=\"Lyricist\"", 
			     rs->GetString(mediadb::LYRICIST));
	    os << IfPresent2("upnp:author", "role=\"OriginalArtist\"", 
			     rs->GetString(mediadb::ORIGINALARTIST));
	}

	if (type == mediadb::RADIO && (filter & CHANNELID))
	{
	    unsigned int service_id = rs->GetInteger(mediadb::PATH);
	    if (service_id)
	    {
		os << "<upnp:channelID type=\"SI\">0,0," << service_id
		   << "</upnp:channelID>";
	    }
	}

	/** @todo Figure out what to do with MOOD.
	 *
	 * @todo AlbumArtURL
	 */

	if (filter & RES)
	{
	    os << ResItem(db, rs, urlprefix, filter);
	    unsigned int idhigh = rs->GetInteger(mediadb::IDHIGH);
	    if (idhigh)
	    {
		db::QueryPtr qp = db->CreateQuery();
		qp->Where(qp->Restrict(mediadb::ID, db::EQ, idhigh));
		rs = qp->Execute();
		if (rs && !rs->IsEOF())
		    os << ResItem(db, rs, urlprefix, filter);
	    }
	}
	os << "</item>";
    }

    return os.str();
}

} // namespace didl
} // namespace mediadb


        /* Unit tests */


#ifdef TEST

# include "libdbsteam/db.h"
# include "libmediadb/xml.h"
# include "libmediadb/localdb.h"

struct TestAttribute
{
    const char *name;
    const char *value;
};

struct TestField
{
    const char *name;
    const char *content;
    const TestAttribute *attributes;
};

struct TestItem
{
    const TestField *fields;
};

struct Test
{
    const char *xml;
    const TestItem *items;
};

const TestAttribute test1resattrs[] = {
    { "protocolInfo", "http-get:*:audio/mpeg:*" },
    { "size",         "6618946" },
    { "duration",     "0:05:40.00" },
    { NULL, NULL }
};

const TestField test1fields[] = {
    { "id",          "22",              NULL },
    { "upnp:artist", "The Stone Roses", NULL },
    { "upnp:genre",  "Indie", NULL },
    { "dc:title",    "I Am The Resurrection (5-3 Stoned Out Club Mix)", NULL },
    { "upnp:class",  "object.item.audioItem.musicTrack", NULL },
    { "res",         "http://192.168.168.15:49152/files/22", test1resattrs },
    { NULL, NULL, NULL }
};

const TestItem test1items[] = {
    { test1fields },
    { NULL }
};

const TestItem test2items[] = {
    { NULL }
};

const Test tests[] = {
    {
	"<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
	" xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
	" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
	"<item id=\"22\" parentID=\"0\" restricted=\"1\">"
	"<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"6618946\""
	" duration=\"0:05:40.00\">http://192.168.168.15:49152/files/22</res>"
	"<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
	"<dc:title>I Am The Resurrection (5-3 Stoned Out Club Mix)</dc:title>"
	"<upnp:artist>The Stone Roses</upnp:artist>"
	"<upnp:genre>Indie</upnp:genre></item>"
	"</DIDL-Lite>",
	test1items
    },

    {
	"<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
	" xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
	" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
	"</DIDL-Lite>",
	test2items
    },
};

enum { TESTS = sizeof(tests)/sizeof(tests[0]) };

int main(int, char**)
{
    /* Test Parse() */

    for (unsigned int i=0; i<TESTS; ++i)
    {
	const Test& test = tests[i];

	mediadb::didl::MetadataList mdl = mediadb::didl::Parse(test.xml);

	mediadb::didl::MetadataList::const_iterator mdli = mdl.begin();

	for (const TestItem *item = test.items; item->fields; ++item)
	{
	    assert(mdli != mdl.end());

	    const mediadb::didl::Metadata& md = *mdli;

	    unsigned int nfields = 0;
	    for (const TestField *field = item->fields; field->name; ++field)
	    {
		++nfields;

		bool found = false;
		for (mediadb::didl::Metadata::const_iterator mdi = md.begin();
		     mdi != md.end();
		     ++mdi)
		{
		    if (mdi->tag == field->name
			&& mdi->content == field->content)
		    {
			found = true;
			if (field->attributes)
			{
			    unsigned int nattrs = 0;
			    for (const TestAttribute *attr = field->attributes;
				 attr->name;
				 ++attr)
			    {
				++nattrs;
				std::map<std::string, std::string>
				    ::const_iterator ci
				    = mdi->attributes.find(attr->name);
				assert(ci != mdi->attributes.end());
				assert(ci->second == attr->value);
			    }

			    if (mdi->attributes.size() != nattrs)
			    {
				TRACE << "Should have " << nattrs
				      << " attributes but have "
				      << mdi->attributes.size() << "\n";
				assert(mdi->attributes.size() == nattrs);
			    }
			}
		    }
		}

		if (!found)
		{
		    TRACE << "Missing tag '" << field->name << "'='"
			  << field->content << "'\n";
		    assert(found);
		}
	    }

	    if (md.size() != nfields)
	    {
		TRACE << "Should have " << nfields << " fields but have "
		      << md.size() << "\n";
		assert(md.size() == nfields);
	    }
	}
    }

    /* Test FromRecord */

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

    db::QueryPtr qp = mdb.CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, 377));
    db::RecordsetPtr rs = qp->Execute();
    assert(rs && !rs->IsEOF());
    std::string didl = mediadb::didl::s_header + mediadb::didl::FromRecord(&mdb, rs)
	+ mediadb::didl::s_footer;

    const char *expected =
	"<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
	" xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
	" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">"
	"<item id=\"377\" parentID=\"0\" restricted=\"1\">"
	"<dc:title>Clean</dc:title>"
	"<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
	"<upnp:artist>Depeche Mode</upnp:artist>"
	"<upnp:album>Violator</upnp:album>"
	"<upnp:originalTrackNumber>9</upnp:originalTrackNumber>"
	"<upnp:genre>Rock</upnp:genre>"
	"<dc:date>1990-01-01</dc:date>"
	"<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"9031623\""
	" duration=\"0:05:32.00\">"
	"file:///home/chorale/mp3/Violator/09%20Clean.mp3"
	"</res>"
	"<res protocolInfo=\"http-get:*:audio/x-flac:*\" size=\"36537605\""
	" duration=\"0:05:32.00\">"
	"file:///home/chorale/flac/Violator/09%20Clean.flac"
	"</res></item></DIDL-Lite>";

    if (didl != expected)
    {
	TRACE << "Expected:\n" << expected << "\nGot:\n" << didl << "\n";
	assert(didl == expected);
    }

    return 0;
}

#endif // TEST
