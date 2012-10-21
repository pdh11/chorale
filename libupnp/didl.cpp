#include "config.h"
#include "didl.h"
#include "libutil/trace.h"
#include "libmediadb/db.h"
#include "libmediadb/schema.h"
#include "libutil/xmlescape.h"
#include <boost/format.hpp>
#include <sstream>

#ifdef HAVE_UPNP
#include <upnp/ixml.h>
#endif

namespace upnp {
namespace didl {

#ifdef HAVE_UPNP

MetadataList Parse(const std::string& xml)
{
    MetadataList results;

    IXML_Document *didl = ixmlParseBuffer(const_cast<char *>(xml.c_str()));
    if (didl)
    {
	IXML_Node *child = ixmlNode_getFirstChild(&didl->n);
	if (child)
	{
	    IXML_NodeList *containers = ixmlNode_getChildNodes(child);
	
	    unsigned int containercount = ixmlNodeList_length(containers);
//	    TRACE << containercount << " container(s)\n";
	
	    for (unsigned int j=0; j<containercount; ++j)
	    {
		Metadata md;
		
		IXML_Node *cnode = ixmlNodeList_item(containers, j);
		if (cnode)
		{
		    IXML_NamedNodeMap *attrs = ixmlNode_getAttributes(cnode);
		    if (attrs)
		    {
			IXML_Node *id = ixmlNamedNodeMap_getNamedItem(attrs,
								      const_cast<char *>("id"));
			if (id)
			{
			    const DOMString ds = ixmlNode_getNodeValue(id);
			    if (ds)
			    {
				Field fd;
				fd.tag = "id";
				fd.content = ds;
				md.push_back(fd);
			    }
			    else
				TRACE << "no value in attr\n";
			}
			else
			    TRACE << "no attr\n";
			ixmlNamedNodeMap_free(attrs);
		    }
		    else
			TRACE << "no attrs\n";

		    IXML_NodeList *nl = ixmlNode_getChildNodes(cnode);
		    
		    unsigned int fieldcount = ixmlNodeList_length(nl);
//		    TRACE << fieldcount << " field(s)\n";
		    
		    for (unsigned int i=0; i<fieldcount; ++i)
		    {
			IXML_Node *node2 = ixmlNodeList_item(nl, i);
			if (node2)
			{
			    const DOMString ds = ixmlNode_getNodeName(node2);
			    if (ds)
			    {
				Field field;
				field.tag = ds;
				IXML_Node *textnode = ixmlNode_getFirstChild(node2);
				if (textnode)
				{
				    const DOMString ds2 = ixmlNode_getNodeValue(textnode);
				    if (ds2)
				    {
//					TRACE << ds << "=" << ds2 << "\n";
				        field.content = ds2;
				    }
				    else
					TRACE << "no value\n";
				}
				else
				    TRACE << "no textnode\n";

				IXML_NamedNodeMap *attrs2 =
				    ixmlNode_getAttributes(node2);
				if (attrs2)
				{
				    unsigned int nattrs =
					ixmlNamedNodeMap_getLength(attrs2);

				    for (unsigned int k=0; k<nattrs; ++k)
				    {
					IXML_Node *attrnode =
					    ixmlNamedNodeMap_item(attrs2, k);
					if (attrnode)
					{
					    const DOMString attrname =
						ixmlNode_getNodeName(attrnode);
					    const DOMString attrvalue =
						ixmlNode_getNodeValue(attrnode);
					    if (attrname)
						field.attributes[attrname]
						    = attrvalue;
					}
				    }

				    ixmlNamedNodeMap_free(attrs2);
				}
				else
				    TRACE << "no attrs\n";

				md.push_back(field);
			    }
			    else
				TRACE << "no name\n";
			}
			else
			    TRACE << "no node\n";
		    }

		    ixmlNodeList_free(nl);
		}
		else
		    TRACE << "no cnode\n";

		results.push_back(md);
	    }

	    ixmlNodeList_free(containers);
	}
	else
	    TRACE << "no child\n";
	
	ixmlDocument_free(didl);
    }
    else
	TRACE << "can't parse didl response\n";

    return results;
}

#endif // HAVE_UPNP

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
			   const char *urlprefix)
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

    unsigned int durationms = rs->GetInteger(mediadb::DURATIONMS);

    unsigned int id = rs->GetInteger(mediadb::ID);
    std::string url = db->GetURL(id);
    if (!strncmp(url.c_str(), "file:", 5) && urlprefix)
	url = (boost::format("%s%x") % urlprefix % id).str();

    url = util::XmlEscape(url);

    return (boost::format("<res protocolInfo=\"http-get:*:%s:*\""
			  " size=\"%u\""
			  " duration=\"%u:%02u:%02u.00\">"
			  "%s</res>")
			  % mimetype
			  % rs->GetInteger(mediadb::SIZEBYTES)
			  % (durationms/3600000)
			  % ((durationms/ 60000) % 60)
	                  % ((durationms/  1000) % 60)
	                  % url).str();
}

const char s_header[] =
    "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
    " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
    " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">";
const char s_footer[] = "</DIDL-Lite>";


/* FromRecord */


std::string FromRecord(mediadb::Database *db, db::RecordsetPtr rs,
		       const char *urlprefix)
{
    std::ostringstream os;
    int id = rs->GetInteger(mediadb::ID);
    int parentid = rs->GetInteger(mediadb::IDPARENT);
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
	    os << "object.container.storageFolder";
	else
	    os << "object.container.playlistContainer";

	os << "</upnp:class>"
	    "<upnp:storageUsed>-1</upnp:storageUsed>";

	if (id == 0)
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

	os << IfPresent("upnp:artist", rs->GetString(mediadb::ARTIST));
	os << IfPresent("upnp:album", rs->GetString(mediadb::ALBUM));
	os << IfPresent("upnp:originalTrackNumber",
			rs->GetInteger(mediadb::TRACKNUMBER));
	os << IfPresent("upnp:genre", rs->GetString(mediadb::GENRE));
	unsigned int y = rs->GetInteger(mediadb::YEAR);
	if (y)
	    os << "<dc:date>" << y << "-01-01</dc:date>";

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

	if (type == mediadb::RADIO)
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

	os << ResItem(db, rs, urlprefix);
	unsigned int idhigh = rs->GetInteger(mediadb::IDHIGH);
	if (idhigh)
	{
	    db::QueryPtr qp = db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, idhigh));
	    rs = qp->Execute();
	    if (rs && !rs->IsEOF())
		os << ResItem(db, rs, urlprefix);
	}
	os << "</item>";
    }

    return os.str();
}

} // namespace didl
} // namespace upnp


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
    }
};

enum { TESTS = sizeof(tests)/sizeof(tests[0]) };

int main(int, char**)
{
#ifdef HAVE_UPNP

    /* Test Parse() */

    for (unsigned int i=0; i<TESTS; ++i)
    {
	const Test& test = tests[i];

	upnp::didl::MetadataList mdl = upnp::didl::Parse(test.xml);

	upnp::didl::MetadataList::const_iterator mdli = mdl.begin();

	for (const TestItem *item = test.items; item->fields; ++item)
	{
	    assert(mdli != mdl.end());

	    const upnp::didl::Metadata& md = *mdli;

	    unsigned int nfields = 0;
	    for (const TestField *field = item->fields; field->name; ++field)
	    {
		++nfields;

		bool found = false;
		for (upnp::didl::Metadata::const_iterator mdi = md.begin();
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

#endif // HAVE_UPNP

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
    std::string didl = upnp::didl::s_header + upnp::didl::FromRecord(&mdb, rs)
	+ upnp::didl::s_footer;

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
