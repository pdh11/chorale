#include "rs.h"
#include "libmediadb/schema.h"
#include "db.h"
#include "libdb/free_rs.h"
#include <sstream>
#include "libutil/trace.h"
#include "libutil/urlescape.h"
#include "libutil/http_fetcher.h"
#include <boost/tokenizer.hpp>

namespace db {
namespace receiver {


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

    if (which == mediadb::TYPE && (m_got_what & GOT_TYPE))
    {
	return m_freers->GetInteger(which);
    }

    if (!m_parent->HasTag(which))
	return 0;

    if ((m_got_what & GOT_TAGS) == 0)
	GetTags();

    return m_freers->GetInteger(which);
}

std::string Recordset::GetString(field_t which)
{
    if (IsEOF())
	return std::string();

    if (which == mediadb::TITLE && (m_got_what & GOT_TITLE))
	return m_freers->GetString(which);

    if (which == mediadb::CHILDREN)
    {
	if ((m_got_what & GOT_CONTENT) == 0)
	    GetContent();
	return m_freers->GetString(which);
    }

    if (!m_parent->HasTag(which))
	return std::string();

    if ((m_got_what & GOT_TAGS) == 0)
	GetTags();

    return m_freers->GetString(which);
}

void Recordset::GetTags()
{
    std::ostringstream os;
    os << "http://" << m_parent->m_ep.ToString() << "/tags/"
       << std::hex << m_id << "?_utf8=1";
    std::string url = os.str();

    std::string content;
    util::http::Fetcher hc(m_parent->m_http, url);
    hc.FetchToString(&content);

    if (!m_freers)
	m_freers = db::FreeRecordset::Create();

    int index = 0;
    while (!content.empty())
    {
	if (content.length() < 2)
	    break; // Malformed

	int id = (unsigned char)content[0];
	unsigned int size = (unsigned char)content[1];
	if (size > content.length() + 2)
	    break; // Malformed

	std::string field(content, 2, size);

	content.erase(0, size+2);
	++index;

	int choraleid = m_parent->m_server_to_media_map[id];
	if (!choraleid || size == 0)
	    continue;
	
//	TRACE << "field " << id << " = '" << field << "'\n";

	switch (choraleid)
	{
	case mediadb::TYPE:
	    if (field == "tune")
		m_freers->SetInteger(mediadb::TYPE, mediadb::TUNE);
	    else if (field == "playlist")
		m_freers->SetInteger(mediadb::TYPE, mediadb::PLAYLIST);
	    else
	    {
		TRACE << "Unrecognised type string '" << field << "' id=" << id << " choraleid=" << choraleid << "\n";
		m_freers->SetInteger(mediadb::TYPE, mediadb::FILE);
	    }
	    break;
	case mediadb::CODEC:
	    if (field == "mp3")
		m_freers->SetInteger(mediadb::CODEC, mediadb::MP3);
	    else if (field == "flac")
		m_freers->SetInteger(mediadb::CODEC, mediadb::FLAC);
	    else
	    {
		TRACE << "Unrecognised codec string '" << field << "'\n";
	    }
	    break;
	default:
	    m_freers->SetString(choraleid, field);
	    break;
	}
    }

    m_got_what |= GOT_TAGS|GOT_TITLE|GOT_TYPE;
}

void Recordset::GetContent()
{
    if (!m_freers)
	m_freers = db::FreeRecordset::Create();
    
    /** @todo Some protection against doing this on a tune */

    std::ostringstream os;
    os << "http://" << m_parent->m_ep.ToString() << "/content/"
       << std::hex << m_id;
    std::string url = os.str();
//    TRACE << "m_id=" << m_id << " url=" << url << "\n";

    std::string content;
    util::http::Fetcher hc(m_parent->m_http, url);
    hc.FetchToString(&content);

    TRACE << "Content is\n" << Hex(content.c_str(), content.length());

    /** Binary playlist format, little-endian words.
     *
     * If (word0 & 0xF) == 0, then each word is an ID.
     *
     * Otherwise, word0 is the format type, and each *pair* of
     * remaining words is a (fid, fid_generation) pair.
     */
    if (content.length() > 4
	&& content[0] == (char)0xFF
	&& content[1] == (char)2)
    {
	std::vector<unsigned int> childvec;
	childvec.resize((content.length()-4) / 8);
	for (unsigned int i=0; i<childvec.size(); ++i)
	{
	    unsigned int offset = 4 + i*8;
	    childvec[i] = (unsigned char)content[offset]
		+ ((unsigned char)content[offset+1] << 8)
		+ ((unsigned char)content[offset+2] << 16)
		+ ((unsigned char)content[offset+3] << 24);
	}

	m_freers->SetString(mediadb::CHILDREN,
			    mediadb::VectorToChildren(childvec));
    }

    m_got_what |= GOT_CONTENT;
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
	m_freers = NULL;
}

bool RecordsetOne::IsEOF()
{
    return m_id == 0;
}


        /* RestrictionRecordset */


RestrictionRecordset::RestrictionRecordset(Database *parent,
			 const Query::Restriction& r)
    : Recordset(parent),
      m_eof(true),
      m_recno(0)
{
    std::ostringstream os;

    // Legacy servers only listen to the *last* X=Y that we pass

    os << "http://" << m_parent->m_ep.ToString() << "/results?"
       << parent->GetFieldName(r.which);
    
    switch (r.rt)
    {
    default:
    case EQ: os << "="; break;
    }

    if (r.is_string)
	os << util::URLEscape(r.sval);
    else 
	os << r.ival;

    os << "&_extended=2&_utf8=1";

    std::string url = os.str();

    TRACE << "restrict url=" << url << "\n";
    
    util::http::Fetcher hc(m_parent->m_http, url);
    hc.FetchToString(&m_content);

    TRACE << "results content (" << m_content.length() << ") =\n"
	  << Hex(m_content.c_str(), m_content.length());
    
    m_eof = m_content.empty();

    if (!m_eof)
	LoadID();
}

void RestrictionRecordset::MoveNext()
{
    ++m_recno;
    if (m_recno >= (m_content.size()/12))
    {
	m_eof = true;
	return;
    }
    LoadID();
}

void RestrictionRecordset::LoadID()
{
    size_t offset = m_recno*12;
    m_id = (unsigned char)m_content[offset]
	+ ((unsigned char)m_content[offset+1] << 8)
	+ ((unsigned char)m_content[offset+2] << 16)
	+ ((unsigned char)m_content[offset+3] << 24);
}

bool RestrictionRecordset::IsEOF()
{
    return m_eof;
}


        /* CollateRecordset */


CollateRecordset::CollateRecordset(Database *parent,
				   const Query::restrictions_t& restrictions,
				   int collateby)
    : m_parent(parent),
      m_recno(0),
      m_eof(true)
{
    std::ostringstream os;

    // Legacy servers only listen to the *last* X=Y that we pass

    os << "http://" << m_parent->m_ep.ToString() << "/query?";

    for (Query::restrictions_t::const_iterator i = restrictions.begin();
	 i != restrictions.end();
	 ++i)
    {
	os << parent->GetFieldName(i->which);
	switch (i->rt)
	{
	default:
	case EQ: os << "="; break;
	}
	if (i->is_string)
	    os << i->sval;
	else 
	    os << i->ival;
	os << "&";
    }

    os << parent->GetFieldName(collateby) << "=&_utf8=1";
    std::string url = os.str();

    TRACE << "collate url=" << url << "\n";

    std::string content;
    util::http::Fetcher hc(m_parent->m_http, url);
    hc.FetchToString(&content);
    
    TRACE << content << "\n";

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

    boost::char_separator<char> sep(":\n");
    
    tokenizer tok(content, sep);

    bool next_ok = false;
    for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i)
    {
	int a,b,c,d;
	if (next_ok)
	{
	    m_values.push_back(*i);
	    next_ok = false;
	}
	else if (sscanf(i->c_str(), "%d=%d,%d,%d", &a, &b, &c, &d) == 4)
	{
	    m_counts.push_back(b);
	    next_ok = true;
	}
	else 
	{
	    TRACE << "don't like line '" << *i << "'\n";
	    next_ok = false;
	}
    }
    m_eof = m_counts.empty();
}

uint32_t CollateRecordset::GetInteger(field_t which)
{
    if (which == 1 && !m_eof)
	return m_counts[m_recno];
    return 0; 
}

std::string CollateRecordset::GetString(field_t which)
{ 
    if (which == 0 && !m_eof)
	return m_values[m_recno];
    return ""; 
}

void CollateRecordset::MoveNext()
{
    ++m_recno;
    if (m_recno >= m_counts.size())
	m_eof = true;
}

bool CollateRecordset::IsEOF()
{
    return m_eof; 
}

} // namespace receiver
} // namespace db
