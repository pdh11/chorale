#include "config.h"
#include "rs.h"
#include "db.h"
#include "libupnp/soap.h"
#include "libupnp/didl.h"
#include "libutil/ssdp.h"
#include "libmediadb/schema.h"
#include "libdb/free_rs.h"
#include "libutil/trace.h"
#include <sstream>
#include <upnp/ixml.h>

namespace db {
namespace upnpav {


        /* Recordset */


Recordset::Recordset(Database *parent)
    : m_parent(parent),
      m_id(0),
      m_got_what(0)
{
}

uint32_t Recordset::GetInteger(int which)
{
    if (which == mediadb::ID)
	return m_id;

    if (IsEOF())
	return 0;

    if (m_got_what & GOT_BASIC)
    {
	if (which == mediadb::TYPE)
	    return m_freers->GetInteger(which);
    }

    if ((m_got_what & GOT_TAGS) == 0)
	GetTags();

    return m_freers->GetInteger(which);
}

std::string Recordset::GetString(int which)
{
    if (IsEOF())
	return std::string();

    if (m_got_what & GOT_BASIC)
    {
	if (which == mediadb::TITLE)
	    return m_freers->GetString(which);
    }

    if (which == mediadb::CHILDREN)
    {
	if ((m_got_what & GOT_CHILDREN) == 0)
	    GetChildren();
	return m_freers->GetString(which);
    }

    if ((m_got_what & GOT_TAGS) == 0)
	GetTags();

    return m_freers->GetString(which);
}

void Recordset::GetTags()
{
    if (!m_freers)
	m_freers = db::FreeRecordset::Create();

    std::string objectid = m_parent->m_idmap[m_id];

    TRACE << "GetTags(" << m_id << ")\n";

    upnp::soap::Connection usc(m_parent->m_control_url,
			       m_parent->m_udn,
			       util::ssdp::s_uuid_contentdirectory);

    upnp::soap::Params in;
    in["ObjectID"] = objectid;
    in["BrowseFlag"] = "BrowseMetadata";
    in["Filter"] = "*";
    in["StartingIndex"] = "0";
    in["RequestedCount"] = "0";
    in["SortCriteria"] = "";

    upnp::soap::Params out = usc.Action("Browse", in);

    upnp::didl::MetadataList ml = upnp::didl::Parse(out["Result"]);

    if (!ml.empty())
    {
	const upnp::didl::Metadata& md = ml.front();
	for (upnp::didl::Metadata::const_iterator i = md.begin();
	     i != md.end();
	     ++i)
	{
	    if (i->tag == "dc:title")
		m_freers->SetString(mediadb::TITLE, i->content);
	    else if (i->tag == "upnp:artist")
		m_freers->SetString(mediadb::ARTIST, i->content);
	    else if (i->tag == "upnp:album")
		m_freers->SetString(mediadb::ALBUM, i->content);
	    else if (i->tag == "upnp:genre")
		m_freers->SetString(mediadb::GENRE, i->content);
	    else if (i->tag == "upnp:originalTrackNumber")
		m_freers->SetInteger(mediadb::TRACKNUMBER, 
				     strtoul(i->content.c_str(), NULL, 10));
	    else if (i->tag == "upnp:class")
	    {
		unsigned int type = mediadb::FILE;

		if (i->content == "object.container.storageFolder"
		    || i->content == "object.container")
		    type = mediadb::DIR;
		else if (i->content == "object.item.audioItem.musicTrack")
		    type = mediadb::TUNE;
		else
		    TRACE << "Unknown class '" << i->content << "'\n";
		
		m_freers->SetInteger(mediadb::TYPE, type);
	    }
	    else if (i->tag == "res")
	    {
		upnp::didl::Attributes::const_iterator ci =
		    i->attributes.find("protocolInfo");
		if (ci != i->attributes.end()
		    && ci->second == "http-get:*:audio/mpeg:*")
		{
		    m_freers->SetInteger(mediadb::CODEC, mediadb::MP3);
		    m_freers->SetString(mediadb::PATH, i->content);
		}
		else
		{
		    TRACE << "Can't find URL:\n" << i->attributes;
		}
	    }
	    else
		TRACE << "Unknown tag '" << i->tag << "'\n";
	}
    }

    m_got_what |= GOT_TAGS|GOT_BASIC;
}

void Recordset::GetChildren()
{
    if (!m_freers)
	m_freers = db::FreeRecordset::Create();

    if ((m_got_what & GOT_BASIC) == 0)
	GetTags();

    if (m_freers->GetInteger(mediadb::TYPE) != mediadb::DIR)
    {
	m_got_what |= GOT_CHILDREN;
	return;
    }

    std::string objectid = m_parent->m_idmap[m_id];

    TRACE << "GetChildren(" << m_id << "[=" << objectid << "])\n";

    upnp::soap::Connection usc(m_parent->m_control_url,
			       m_parent->m_udn,
			       util::ssdp::s_uuid_contentdirectory);

    upnp::soap::Params in;
    in["ObjectID"] = objectid;
    in["BrowseFlag"] = "BrowseDirectChildren";
    in["Filter"] = "";
    in["StartingIndex"] = "0";
    in["RequestedCount"] = "0";
    in["SortCriteria"] = "";

    upnp::soap::Params out = usc.Action("Browse", in);

    upnp::didl::MetadataList ml = upnp::didl::Parse(out["Result"]);

    std::vector<unsigned int> childvec;

    for (upnp::didl::MetadataList::iterator i = ml.begin();
	 i != ml.end();
	 ++i)
    {
	for (upnp::didl::Metadata::iterator j = i->begin();
	     j != i->end();
	     ++j)
	{
	    if (j->tag == "id")
	    {
		std::string childid = j->content;
		unsigned int id = m_parent->IdForObjectId(childid);
		TRACE << "child '" << childid << "' = " << id << "\n";
		childvec.push_back(id);
		break;
	    }
	}
    }

    m_freers->SetString(mediadb::CHILDREN,
			mediadb::VectorToChildren(childvec));

    m_got_what |= GOT_CHILDREN;
}


        /* RecordsetOne */


RecordsetOne::RecordsetOne(Database *parent, unsigned int id)
    : Recordset(parent)
{
    m_id = id;
}

void RecordsetOne::MoveNext()
{
    m_id = 0;
    m_got_what = 0;
    if (m_freers)
	m_freers.reset();
}

bool RecordsetOne::IsEOF()
{
    return m_id == 0;
}

}; // namespace upnpav
}; // namespace db
