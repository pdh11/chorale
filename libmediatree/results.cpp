#include "results.h"
#include "directory.h"
#include "node_enumerator.h"
#include "libdb/query.h"
#include "libmediadb/schema.h"
#include "libdb/free_rs.h"

namespace mediatree {

Results::Results(db::Database *thedb, int field, const std::string& value)
    : m_db(thedb),
      m_field(field),
      m_value(value),
      m_info(db::FreeRecordset::Create())
{
    m_info->SetString(mediadb::TITLE, value);
    m_info->SetInteger(mediadb::TYPE, mediadb::QUERY);
}

Results::~Results()
{
}

NodePtr Results::Create(db::Database *thedb, int field, 
			const std::string& value)
{
    return NodePtr(new Results(thedb, field, value));
}

std::string Results::GetName()
{
    return m_info->GetString(mediadb::TITLE);
}

bool Results::HasCompoundChildren()
{
    return false;
}

Node::EnumeratorPtr Results::GetChildren()
{
    if (m_children.empty())
    {
	db::QueryPtr qp = m_db->CreateQuery();
	qp->Where(qp->Restrict(m_field, db::EQ, m_value));
	db::RecordsetPtr rs = qp->Execute();

	while (rs && !rs->IsEOF())
	{
	    m_children.push_back(Directory::Create(m_db,
						   rs->GetInteger(mediadb::ID)));
	    rs->MoveNext();
	}
    }

    return Node::EnumeratorPtr(new ContainerEnumerator<std::vector<NodePtr> >(m_children));
}

db::RecordsetPtr Results::GetInfo()
{
    return m_info;
}

} // namespace mediatree
