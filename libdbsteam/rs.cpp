#include "config.h"
#include "rs.h"
#include "db.h"
#include "query.h"
#include "libutil/trace.h"
#include <boost/format.hpp>
#include <errno.h>

namespace db {
namespace steam {

Recordset::Recordset(Database *db)
    : m_db(db),
      m_record(0),
      m_eof(false)
{
    if (m_db->m_data.empty())
	m_eof = true;
    else
    {
	boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);
	m_record = m_db->m_data.begin()->first;
    }
}

void Recordset::SetRecordNumber(unsigned int which)
{
    m_record = which;

    // Caller is assumed to know what they're doing
    m_eof = false;
}

bool Recordset::IsEOF()
{
    return m_eof;
}

uint32_t Recordset::GetInteger(field_t which)
{
    if (m_eof)
	return 0;

    boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);

    Database::records_t::iterator i = m_db->m_data.find(m_record);
    if (i == m_db->m_data.end())
	return 0;
    Database::FieldValue& v = i->second[which];
    if (v.ivalid)
	return v.i;
    if (v.svalid)
    {
	v.ivalid = 1;
	return v.i = (uint32_t)strtoul(v.s.c_str(), NULL, 10);
    }
    return 0;
}

std::string Recordset::GetString(field_t which)
{
    if (m_eof)
    {
	TRACE << "GetString(" << which << ") while EOF\n";
	return "";
    }

    boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);

    Database::records_t::iterator i = m_db->m_data.find(m_record);
    if (i == m_db->m_data.end())
    {
	TRACE << "GetString m_record=" << m_record << " not found\n";
	return "";
    }
    Database::FieldValue& v = i->second[which];
    if (v.svalid)
	return v.s;
    if (v.ivalid && v.i)
	return (boost::format("%u") % v.i).str();

    return "";
}

unsigned int Recordset::SetString(field_t which, const std::string& s)
{
    if (m_eof)
    {
	TRACE << "SetString(" << which << "," << s << ") eof\n";
	return ENOENT;
    }

    boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);

    Database::records_t::iterator i = m_db->m_data.find(m_record);
    if (i == m_db->m_data.end())
    {
	TRACE << "SetString(" << which << "," << s << ") recno " << m_record << " not found\n";
	return ENOENT;
    }
    Database::FieldValue& v = i->second[which];

    if (v.svalid && v.s == s)
	return 0;

    // Update indexes
    unsigned int flags = m_db->m_fields[which].flags;
    if (flags & FIELD_INDEXED)
    {
	switch (flags & FIELD_TYPEMASK)
	{
	default:
	case FIELD_STRING:
	    if (v.svalid)
	    {
		m_db->m_stringindexes[which][v.s].erase(m_record);
		if (m_db->m_stringindexes[which][v.s].empty())
		    m_db->m_stringindexes[which].erase(v.s);
	    }
	    m_db->m_stringindexes[which][s].insert(m_record);
	    v.ivalid = 0;
	    break;
	case FIELD_INT:
	    if (v.ivalid)
	    {
		m_db->m_intindexes[which][v.i].erase(m_record);
		if (m_db->m_intindexes[which][v.i].empty())
		    m_db->m_intindexes[which].erase(v.i);
	    }
	    v.i = (uint32_t)strtoul(s.c_str(), NULL, 10);
	    v.ivalid = 1;
	    m_db->m_intindexes[which][v.i].insert(m_record);
	    break;
	}
    }

    v.s = s;
    v.svalid = 1;
    return 0;
}

unsigned int Recordset::SetInteger(field_t which, uint32_t n)
{
    if (m_eof)
	return ENOENT;

    boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);

    Database::records_t::iterator i = m_db->m_data.find(m_record);
    if (i == m_db->m_data.end())
	return ENOENT;
    Database::FieldValue& v = i->second[which];

    // Update indexes
    unsigned int flags = m_db->m_fields[which].flags;
    if (flags & FIELD_INDEXED)
    {
	switch (flags & FIELD_TYPEMASK)
	{
	default:
	case FIELD_INT:
	    if (v.ivalid)
	    {
		m_db->m_intindexes[which][v.i].erase(m_record);
		if (m_db->m_intindexes[which][v.i].empty())
		    m_db->m_intindexes[which].erase(v.i);
	    }
	    m_db->m_intindexes[which][n].insert(m_record);
	    v.svalid = 0;
	    break;
	case FIELD_STRING:
	    if (v.svalid)
	    {
		m_db->m_stringindexes[which][v.s].erase(m_record);
		if (m_db->m_stringindexes[which][v.s].empty())
		    m_db->m_stringindexes[which].erase(v.s);
	    }
	    v.s = (boost::format("%u") % n).str();
	    v.svalid = 1;
	    m_db->m_stringindexes[which][v.s].insert(m_record);
	    break;
	}
    }

    v.i = n;
    v.ivalid = 1;
    return 0;
}

unsigned int Recordset::AddRecord()
{
//    TRACE << "Adding record " << m_db->m_next_recno << "\n";

    boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);

    m_record = m_db->m_next_recno;
    m_db->m_data[m_db->m_next_recno++].resize(m_db->m_nfields);
    m_eof = false;
    return 0;
}

unsigned int Recordset::Commit()
{
    return 0;
}

unsigned int Recordset::Delete()
{
    if (m_eof)
	return ENOENT;

    {
	boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);

	Database::records_t::iterator iter = m_db->m_data.find(m_record);
	if (iter == m_db->m_data.end())
	    return ENOENT;

	// Remove from any indexes
	for (unsigned int i=0; i<m_db->m_nfields; ++i)
	{
	    unsigned int flags = m_db->m_fields[i].flags;
	    Database::FieldValue& v = iter->second[i];
	    if (flags & FIELD_INDEXED)
	    {
		switch (flags & FIELD_TYPEMASK)
		{
		default:
		case FIELD_INT:
		    if (v.ivalid)
		    {
			m_db->m_intindexes[i][v.i].erase(m_record);
			if (m_db->m_intindexes[i][v.i].empty())
			    m_db->m_intindexes[i].erase(v.i);
		    }
		    break;
		case FIELD_STRING:
		    if (v.svalid)
		    {
			m_db->m_stringindexes[i][v.s].erase(m_record);
			if (m_db->m_stringindexes[i][v.s].empty())
			    m_db->m_stringindexes[i].erase(v.s);
		    }
		    break;
		}
	    }
	}

	// Delete record itself
	m_db->m_data.erase(m_record);
    }

    MoveNext();

    return 0;
}


        /* SimpleRecordset */


SimpleRecordset::SimpleRecordset(Database *db, Query *q)
    : Recordset(db),
      m_query(q)
{
//    TRACE << "SRS constructor, eof=" << m_eof << "\n";
    if (!m_eof && (m_query && !m_query->Match(this)))
	MoveNext();
}

void SimpleRecordset::MoveNext()
{
    if (m_eof)
	return;

    for (;;)
    {
	// Finds first item with key greater than m_record
	Database::records_t::iterator i = m_db->m_data.upper_bound(m_record);
	if (i == m_db->m_data.end())
	{
	    m_eof = true;
	    return;
	}
	m_record = i->first;
	if (!m_query || m_query->Match(this))
	    return;
    }
}


        /* IndexedRecordset */


IndexedRecordset::IndexedRecordset(Database *db, field_t field, uint32_t intval)
    : Recordset(db),
      m_field(field),
      m_is_int(true),
      m_intval(intval),
      m_subrecno(0)
{
    boost::recursive_mutex::scoped_lock lock(db->m_mutex);
    Database::intindex_t& the_index = db->m_intindexes[field];
    Database::intindex_t::const_iterator i = the_index.find(intval);
    if (i == the_index.end())
	m_eof = true;
    else
    {
	m_eof = false;
	m_record = *(i->second.begin());
    }
}

IndexedRecordset::IndexedRecordset(Database *db, field_t field,
				   std::string stringval)
    : Recordset(db),
      m_field(field),
      m_is_int(false),
      m_stringval(stringval),
      m_subrecno(0)
{
    boost::recursive_mutex::scoped_lock lock(db->m_mutex);
    Database::stringindex_t& the_index = db->m_stringindexes[field];
    Database::stringindex_t::const_iterator i = the_index.find(stringval);
    if (i == the_index.end())
    {
	m_eof = true;
    }
    else
    {
	m_eof = false;
	m_record = *(i->second.begin());
    }
}

void IndexedRecordset::MoveNext()
{
    if (m_eof)
	return;

    boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);
    if (m_is_int)
    {
	Database::intindex_t& the_index = m_db->m_intindexes[m_field];
	Database::intindex_t::const_iterator i = the_index.find(m_intval);
	if (i == the_index.end())
	    m_eof = true;
	else
	{
	    ++m_subrecno;
	    if (m_subrecno >= i->second.size())
		m_eof = true;
	    else
	    {
		std::set<unsigned int>::const_iterator ci = i->second.begin();
		for (unsigned int j=0; j<m_subrecno; ++j)
		    ++ci;
		m_record = *ci;
	    }
	}
    }
    else
    {
	// String case
	Database::stringindex_t& the_index = m_db->m_stringindexes[m_field];
	Database::stringindex_t::const_iterator i = the_index.find(m_stringval);
	if (i == the_index.end())
	{
	    m_eof = true;
	    return;
	}

	++m_subrecno;
	if (m_subrecno >= i->second.size())
	{
	    m_eof = true;
	    return;
	}

	std::set<unsigned int>::const_iterator ci = i->second.begin();
	for (unsigned int j=0; j<m_subrecno; ++j)
	    ++ci;
	m_record = *ci;
    }
}


        /* OrderedRecordset */


OrderedRecordset::OrderedRecordset(Database *db, field_t field)
    : Recordset(db),
      m_field(field)
{
    boost::recursive_mutex::scoped_lock lock(db->m_mutex);
    m_is_int = ((db->m_fields[field].flags & FIELD_TYPEMASK)
		== FIELD_INT);
    if (m_is_int)
    {
	Database::intindex_t& the_index = db->m_intindexes[field];
	Database::intindex_t::const_iterator i = the_index.begin();
	if (i == the_index.end())
	    m_eof = true;
	else
	{
	    m_eof = false;
	    m_record = *(i->second.begin());
	    m_subrecno = 0;
	    m_intval = i->first;
	}
    }
    else
    {
	Database::stringindex_t& the_index = db->m_stringindexes[field];
	Database::stringindex_t::const_iterator i = the_index.begin();
	if (i == the_index.end())
	    m_eof = true;
	else
	{
	    m_eof = false;
	    m_record = *(i->second.begin());
	    m_subrecno = 0;
	    m_stringval = i->first;
	}
    }
}

void OrderedRecordset::MoveNext()
{
    if (m_eof)
	return;

    boost::recursive_mutex::scoped_lock lock(m_db->m_mutex);
    if (m_is_int)
    {
	Database::intindex_t& the_index = m_db->m_intindexes[m_field];
	Database::intindex_t::const_iterator i = the_index.lower_bound(m_intval);
	if (i == the_index.end())
	    m_eof = true;
	else
	{
	    if (i->first != m_intval)
		m_subrecno = 0;
	    else
		++m_subrecno;

	    while (m_subrecno >= i->second.size())
	    {
		++i;
		if (i == the_index.end())
		{
		    m_eof = true;
		    return;
		}
		m_subrecno = 0;
	    }

	    m_intval = i->first;
	    
	    std::set<unsigned int>::const_iterator ci = i->second.begin();
	    for (unsigned int j=0; j<m_subrecno; ++j)
		++ci;
	    m_record = *ci;
	}
    }
    else
    {
	// String case
	Database::stringindex_t& the_index = m_db->m_stringindexes[m_field];
	Database::stringindex_t::const_iterator i = the_index.lower_bound(m_stringval);
	if (i == the_index.end())
	{
	    m_eof = true;
	}
	else
	{
	    if (i->first != m_stringval)
		m_subrecno = 0;
	    else
		++m_subrecno;

	    while (m_subrecno >= i->second.size())
	    {
		++i;
		if (i == the_index.end())
		{
		    m_eof = true;
		    return; 
		}
		m_subrecno = 0;
	    }

	    m_stringval = i->first;
	    
	    std::set<unsigned int>::const_iterator ci = i->second.begin();
	    for (unsigned int j=0; j<m_subrecno; ++j)
		++ci;
	    m_record = *ci;
	}
    }
}


        /* CollateRecordset */


CollateRecordset::CollateRecordset(Database *db, field_t field, Query *query)
    : m_parent(db), m_field(field), m_eof(false), m_query(query), m_rs(db)
{
    boost::recursive_mutex::scoped_lock lock(m_parent->m_mutex);
    m_is_int = ((m_parent->m_fields[field].flags & FIELD_TYPEMASK)
		== FIELD_INT);
    if (m_is_int)
    {
	if (m_parent->m_intindexes[field].empty())
	    m_eof = true;
	else
	{
	    m_intvalue = m_parent->m_intindexes[field].begin()->first;
	    m_eof = false;
	}
    }
    else
    {
	// String
	if (m_parent->m_stringindexes[field].empty())
	    m_eof = true;
	else
	{
	    const Database::stringindex_t& index
		= m_parent->m_stringindexes[m_field];

	    MoveUntilValid(index.begin(), index.end());
	}
    }
}

void CollateRecordset::MoveUntilValid(Database::stringindex_t::const_iterator i,
				      Database::stringindex_t::const_iterator end)
{
    for (;;)
    {
	if (i == end)
	{
	    m_eof = true;
	    return;
	}

	m_strvalue = i->first;

	if (!m_query)
	{
	    // They're all valid then
	    return;
	}

	/** @todo Optimisation: if all restrictions are on the collate field,
	 *        we only need to examine one record.
	 */
	for (std::set<unsigned int>::const_iterator ci = i->second.begin();
	     ci != i->second.end();
	     ++ci)
	{
	    m_rs.SetRecordNumber(*ci);
	    if (m_query->Match(&m_rs))
		return; // Found an acceptable one
	}

	++i;
    }
}

bool CollateRecordset::IsEOF() 
{
    return m_eof; 
}

uint32_t CollateRecordset::GetInteger(field_t)
{
    return m_intvalue;
}

std::string CollateRecordset::GetString(field_t)
{
    return m_strvalue; 
}

void CollateRecordset::MoveNext()
{
    if (m_eof)
	return;

    boost::recursive_mutex::scoped_lock lock(m_parent->m_mutex);
    if (m_is_int)
    {
	const Database::intindex_t& index = m_parent->m_intindexes[m_field];
	Database::intindex_t::const_iterator i = index.upper_bound(m_intvalue);
	if (i == index.end())
	{
	    m_eof = true;
	    return;
	}
	m_intvalue = i->first;
    }
    else
    {
	const Database::stringindex_t& index
	    = m_parent->m_stringindexes[m_field];
	Database::stringindex_t::const_iterator i = index.find(m_strvalue);
	if (i == index.end())
	{
	    m_eof = true;
	    return;
	}

	++i;

	MoveUntilValid(i, index.end());
    }
}

} // namespace steam
} // namespace db

#ifdef TEST

int main()
{
    db::steam::Test();
    return 0;
}

#endif
