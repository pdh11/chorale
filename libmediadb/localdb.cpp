#include "localdb.h"
#include "schema.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include <fcntl.h>

namespace mediadb {

db::RecordsetPtr LocalDatabase::CreateRecordset()
{
    return m_db->CreateRecordset();
}

db::QueryPtr LocalDatabase::CreateQuery()
{
    return m_db->CreateQuery();
}

std::string LocalDatabase::GetURL(unsigned int id)
{
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return std::string();
    }

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    factories_t::const_iterator i = m_factories.find(type);
    if (i != m_factories.end())
	return i->second->GetURL(rs);

    return util::PathToURL(rs->GetString(PATH));
}

util::SeekableStreamPtr LocalDatabase::OpenRead(unsigned int id)
{
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return util::SeekableStreamPtr();
    }

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    factories_t::const_iterator i = m_factories.find(type);
    if (i != m_factories.end())
	return i->second->OpenRead(rs);
    
    util::SeekableStreamPtr fsp;
    unsigned int rc = util::OpenFileStream(rs->GetString(PATH).c_str(),
					   util::READ, &fsp);
    if (rc)
    {
	TRACE << "Can't open ID " << id << " file " << rs->GetString(PATH)
	      << " rc=" << rc << "\n";
	return util::SeekableStreamPtr();
    }
    return fsp;
}

util::SeekableStreamPtr LocalDatabase::OpenWrite(unsigned int id)
{
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't open ID " << id << " for reading\n";
	return util::SeekableStreamPtr();
    }

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    factories_t::const_iterator i = m_factories.find(type);
    if (i != m_factories.end())
	return i->second->OpenWrite(rs);
    
    util::SeekableStreamPtr fsp;
    unsigned int rc = util::OpenFileStream(rs->GetString(PATH).c_str(),
					   util::WRITE, &fsp);
    if (rc)
    {
	TRACE << "Can't open ID " << id << " file " << rs->GetString(PATH)
	      << " rc=" << rc << "\n";
	return util::SeekableStreamPtr();
    }
    return fsp;
}

} // namespace mediadb
