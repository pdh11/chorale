#include "config.h"
#include "rs.h"
#include "db.h"
#include "libmediadb/didl.h"
#include "libupnp/ContentDirectory3.h"
#include "libmediadb/schema.h"
#include "libdb/free_rs.h"
#include "libutil/trace.h"
#include <sstream>
#include <errno.h>

namespace db {
namespace upnpav {


        /* Recordset */


Recordset::Recordset(Database *parent)
    : m_parent(parent),
      m_id(0),
      m_got_what(0)
{
}

uint32_t Recordset::GetInteger(field_t which)
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

std::string Recordset::GetString(field_t which)
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

    std::string objectid = m_parent->ObjectIdForId(m_id);

//    TRACE << "GetTags(" << m_id << ")\n";

    std::string result;
    m_parent->GetContentDirectory()->Browse(objectid,
					    upnp::ContentDirectory3::BROWSEFLAG_BROWSE_METADATA,
					    "*",
					    0, 0, "", &result,
					    NULL, NULL, NULL);
    mediadb::didl::MetadataList ml = mediadb::didl::Parse(result);

    if (!ml.empty())
	mediadb::didl::ToRecord(ml.front(), m_freers);

    m_got_what |= GOT_TAGS|GOT_BASIC;
}

void Recordset::GetChildren()
{
    if (!m_freers)
	m_freers = db::FreeRecordset::Create();

    if ((m_got_what & GOT_BASIC) == 0)
	GetTags();

    unsigned int type = m_freers->GetInteger(mediadb::TYPE);
    if (type != mediadb::DIR && type != mediadb::PLAYLIST)
    {
	m_got_what |= GOT_CHILDREN;
	return;
    }

    Database::Lock lock(m_parent);

    m_parent->m_infomap.clear();

    std::string objectid = m_parent->ObjectIdForId(m_id);

//    TRACE << "GetChildren(" << m_id << "[=" << objectid << "])\n";

    /* We only want objectids, which are obligatory, so we pass filter "".
     *
     * Intel libupnp only allows a certain maximum size of a SOAP response, so
     * in case the list is very long we ask for it in clumps.
     */
    enum { LUMP = 32 };

    std::vector<unsigned int> childvec;

    for (unsigned int start=0; ; start += LUMP)
    {
	std::string result;
	uint32_t nret = 0;
	uint32_t total = 0;
	unsigned int rc = m_parent->GetContentDirectory()
	    ->Browse(objectid,
		     upnp::ContentDirectory3::BROWSEFLAG_BROWSE_DIRECT_CHILDREN,
		     "", start, LUMP, "",
		     &result, &nret, &total, NULL);
	if (rc == 0)
	{
	    mediadb::didl::MetadataList ml = mediadb::didl::Parse(result);

	    for (mediadb::didl::MetadataList::iterator i = ml.begin();
		 i != ml.end();
		 ++i)
	    {
		Database::BasicInfo bi;
		unsigned int id = 0;
		for (mediadb::didl::Metadata::iterator j = i->begin();
		     j != i->end();
		     ++j)
		{
		    if (j->tag == "id")
		    {
			std::string childid = j->content;
			id = m_parent->IdForObjectId(childid);
//			TRACE << "child '" << childid << "' = " << id << "\n";
			childvec.push_back(id);
			break;
		    }
		}
		
		if (id)
		{
		    db::RecordsetPtr child_rs = db::FreeRecordset::Create();
		    mediadb::didl::ToRecord(*i, child_rs);
		    bi.type = child_rs->GetInteger(mediadb::TYPE);
		    bi.title = child_rs->GetString(mediadb::TITLE);
		    m_parent->m_infomap[id] = bi;
		}
	    }
	}
	else
	    break;

	if (start + nret == total)
	    break;
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

    Database::Lock lock(m_parent);

    Database::infomap_t::const_iterator ci = parent->m_infomap.find(id);
    if (ci != parent->m_infomap.end())
    {
	if (!m_freers)
	    m_freers = db::FreeRecordset::Create();

	m_freers->SetInteger(mediadb::TYPE, ci->second.type);
	m_freers->SetString(mediadb::TITLE, ci->second.title);
	m_got_what |= GOT_BASIC;
    }
}

void RecordsetOne::MoveNext()
{
    m_id = 0;
    m_got_what = 0;
    if (m_freers)
	m_freers = NULL;
}

bool RecordsetOne::IsEOF()
{
    return m_id == 0;
}


        /* CollateRecordset */


CollateRecordset::CollateRecordset(Database *db, field_t field)
    : m_parent(db),
      m_field(field),
      m_start(0),
      m_index(0),
      m_total(0)
{
    GetSome();
}

unsigned int CollateRecordset::GetSome()
{
    m_start += m_index;
    m_index = 0;
    m_items.clear();
    m_items.reserve(LUMP);

    std::string query = "upnp:class = \"";
    switch (m_field)
    {
    case mediadb::ARTIST:
	query += "object.container.person.musicArtist";
	break;
    case mediadb::ALBUM:
	query += "object.container.album.musicAlbum";
	break;
    case mediadb::GENRE:
	query += "object.container.genre.musicGenre";
	break;
    default:
	TRACE << "Can't do collate query on #" << m_field << "\n";
	return EINVAL;
    }
    query += "\"";

    std::string result;
    uint32_t n;
    uint32_t total;

    unsigned int rc = m_parent->m_contentdirectory.Search("0", query, "*",
							  m_start, LUMP, "",
							  &result, &n,
							  &total, NULL);
    if (rc != 0)
	return rc;

    if (total)
	m_total = total;
    else
    {
	if (!n)
	    m_total = m_start; // EOF
	else // There are more, but the server can't tell us how many
	    m_total = m_start + n + 1;
    }

    mediadb::didl::MetadataList ml = mediadb::didl::Parse(result);
    for (mediadb::didl::MetadataList::const_iterator i = ml.begin();
	 i != ml.end();
	 ++i)
    {
	const mediadb::didl::Metadata& md = *i;
	for (mediadb::didl::Metadata::const_iterator j = md.begin();
	     j != md.end();
	     ++j)
	{
	    if (j->tag == "dc:title")
	    {
		m_items.push_back(j->content);
		break;
	    }
	}
    }

//    TRACE << "Got " << m_items.size() << " items, expected " << n << "\n";
    
    if (m_items.size() != n)
    {
	TRACE << "Didn't find " << n << " titles in " << result << "\n";
    }
    if (m_items.empty())
	m_total = m_start;
    return 0;
}

bool CollateRecordset::IsEOF()
{
    return m_start + m_index == m_total;
}

void CollateRecordset::MoveNext()
{
    if (!IsEOF())
    {
	++m_index;
	if (m_index == m_items.size() && !IsEOF())
	    GetSome();
    }
}

std::string CollateRecordset::GetString(field_t)
{
    if (IsEOF())
	return std::string();
    return m_items[m_index];
}

uint32_t CollateRecordset::GetInteger(field_t)
{
    TRACE << "Warning, GetInteger on collation not implemented\n";
    return 0;
}


        /* SearchRecordset */


SearchRecordset::SearchRecordset(Database *db, const std::string& query)
    : Recordset(db),
      m_query(query),
      m_start(0),
      m_index(0),
      m_total(0)
{
    GetSome();
    SelectThisItem();
}

unsigned int SearchRecordset::GetSome()
{
    m_start += m_index;
    m_index = 0;
    m_items.clear();

    std::string result;
    uint32_t n;
    uint32_t total;

    unsigned int rc = m_parent->m_contentdirectory.Search("0", m_query, "", 
							  m_start, LUMP, "",
							  &result, &n,
							  &total, NULL);
    if (rc != 0)
	return rc;

    if (total)
	m_total = total;
    else
    {
	if (!n)
	    m_total = m_start; // EOF
	else // There are more, but the server can't tell us how many
	    m_total = m_start + n + 1;
    }

    m_items = mediadb::didl::Parse(result);

//    TRACE << "Got " << m_items.size() << " items, expected " << n << "\n";
    
    if (m_items.size() != n)
    {
	TRACE << "Didn't find " << n << " items in " << result << "\n";
    }
    if (m_items.empty())
	m_total = m_start;

    return 0;
}

void SearchRecordset::SelectThisItem()
{
    if (IsEOF())
	return;

    mediadb::didl::MetadataList::const_iterator ci = m_items.begin();

    for (unsigned int i=0; i<m_index; ++i)
	++ci;

    std::string objectid;
    for (mediadb::didl::Metadata::const_iterator i = ci->begin();
	 i != ci->end();
	 ++i)
	if (i->tag == "id")
	{
	    objectid = i->content;
	    break;
	}

    m_freers = db::FreeRecordset::Create();
    mediadb::didl::ToRecord(*ci, m_freers);
    m_id = m_parent->IdForObjectId(objectid);
    m_got_what = GOT_BASIC;
}

bool SearchRecordset::IsEOF()
{
    return m_start + m_index == m_total;
}

void SearchRecordset::MoveNext()
{
    if (!IsEOF())
    {
	++m_index;
	if (m_index == m_items.size() && !IsEOF())
	    GetSome();
	SelectThisItem();
    }
}

} // namespace upnpav
} // namespace db
