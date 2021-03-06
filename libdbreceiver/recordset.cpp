#include "recordset.h"
#include "connection.h"
#include "libmediadb/schema.h"
#include "libdb/free_rs.h"
#include <sstream>
#include "libutil/trace.h"
#include "libutil/urlescape.h"
#include "libutil/http_fetcher.h"
#include <boost/tokenizer.hpp>
#include <stdio.h>

namespace db {
namespace receiver {


        /* Recordset */


Recordset::Recordset(Connection *parent)
    : m_parent(parent),
      m_id(0),
      m_got_what(0)
{
}

uint32_t Recordset::GetInteger(unsigned int which) const
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

std::string Recordset::GetString(unsigned int which) const
{
    if (IsEOF())
	return std::string();

    if (which == mediadb::TITLE && (m_got_what & GOT_TITLE))
	return m_freers->GetString(which);

    if (which == mediadb::CHILDREN)
    {
	if ((m_got_what & GOT_TAGS) == 0)
	    GetTags();
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

void Recordset::GetTags() const
{
    std::ostringstream os;
    os << "http://" << m_parent->m_ep.ToString() << "/tags/"
       << std::hex << m_id << "?_utf8=1";
    std::string url = os.str();

//    TRACE << "Asking for URL " << url << "\n";

    std::string content;
    util::http::Fetcher hc(m_parent->m_http, url);
    hc.FetchToString(&content);

//    TRACE << "Content of " << url << " is\n" << Hex(content.c_str(), content.length());

    if (!m_freers)
	m_freers = db::FreeRecordset::Create();

    while (!content.empty())
    {
	if (content.length() < 2)
	{
//	    TRACE << "Don't like this content (length=" << content.length()
//		  << ")\n";
	    break; // Malformed
	}

	int id = (unsigned char)content[0];
	unsigned int size = (unsigned char)content[1];
	if (size > content.length() + 2)
	{
//	    TRACE << "Don't like this field (size=" << size
//		  << ", content length=" << content.length() << ")\n";
	    break; // Malformed
	}

	std::string field(content, 2, size);

	content.erase(0, size+2);

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
//	    TRACE << "id " << m_id << " type is "
//		  << m_freers->GetInteger(mediadb::TYPE) << "\n";
	    break;
	case mediadb::AUDIOCODEC:
	    if (field == "mp3")
		m_freers->SetInteger(mediadb::AUDIOCODEC, mediadb::MP3);
	    else if (field == "flac")
		m_freers->SetInteger(mediadb::AUDIOCODEC, mediadb::FLAC);
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

void Recordset::GetContent() const
{
    if (!m_freers)
	m_freers = db::FreeRecordset::Create();
    
    unsigned int type = m_freers->GetInteger(mediadb::TYPE);
    if (type != mediadb::PLAYLIST && type != mediadb::DIR)
    {
	TRACE << "Content asked-for on id " << m_id 
	      << " with non-playlist type " << type << "\n";
	m_got_what |= GOT_CONTENT;
	return;
    }

    std::ostringstream os;
    os << "http://" << m_parent->m_ep.ToString() << "/content/"
       << std::hex << m_id;
    std::string url = os.str();
//    TRACE << "m_id=" << m_id << " url=" << url << "\n";

    std::string content;
    util::http::Fetcher hc(m_parent->m_http, url);
    hc.FetchToString(&content);

//    TRACE << "Content of " << m_id << " is\n" << util::Hex(content.c_str(), content.length());

    std::vector<unsigned int> childvec;

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
	childvec.resize((content.length()-4) / 8);
	for (unsigned int i=0; i<childvec.size(); ++i)
	{
	    unsigned int offset = 4 + i*8;
	    childvec[i] = (unsigned char)content[offset]
		+ ((unsigned char)content[offset+1] << 8)
		+ ((unsigned char)content[offset+2] << 16)
		+ ((unsigned char)content[offset+3] << 24);
	}
    }
    else
    {
	childvec.resize(content.length() / 4);
	for (unsigned int i=0; i<childvec.size(); ++i)
	{
	    unsigned int offset = i*4;
	    childvec[i] = (unsigned char)content[offset]
		+ ((unsigned char)content[offset+1] << 8)
		+ ((unsigned char)content[offset+2] << 16)
		+ ((unsigned char)content[offset+3] << 24);
	}
    }
  
    m_freers->SetString(mediadb::CHILDREN,
			mediadb::VectorToChildren(childvec));

    m_got_what |= GOT_CONTENT;
}


        /* RecordsetOne */


RecordsetOne::RecordsetOne(Connection *parent, unsigned int id)
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

bool RecordsetOne::IsEOF() const
{
    return m_id == 0;
}


        /* RestrictionRecordset */


RestrictionRecordset::RestrictionRecordset(Connection *parent,
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
    case NE: os << "!="; break;
    case GT: os << ">"; break;
    case LT: os << "<"; break;
    case GE: os << ">="; break;
    case LE: os << "<="; break;
    case LIKE: os << "~="; break;
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
	  << util::Hex(m_content.c_str(), m_content.length());
    
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

bool RestrictionRecordset::IsEOF() const
{
    return m_eof;
}


        /* CollateRecordset */


CollateRecordset::CollateRecordset(Connection *parent,
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
	case NE: os << "!="; break;
	case GT: os << ">"; break;
	case LT: os << "<"; break;
	case GE: os << ">="; break;
	case LE: os << "<="; break;
	case LIKE: os << "~="; break;
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

uint32_t CollateRecordset::GetInteger(unsigned int which) const
{
    if (which == 1 && !m_eof)
	return m_counts[m_recno];
    return 0; 
}

std::string CollateRecordset::GetString(unsigned int which) const
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

bool CollateRecordset::IsEOF() const
{
    return m_eof; 
}

} // namespace receiver
} // namespace db
