#include "query.h"
#include "results.h"
#include "libdb/query.h"
#include "libutil/trace.h"
#include "libdb/free_rs.h"
#include "libmediadb/schema.h"
#include "node_enumerator.h"

namespace mediatree {

Query::Query(db::Database *thedb, int field, const std::string& name)
    : m_db(thedb),
      m_field(field),
      m_info(db::FreeRecordset::Create())
{
    m_info->SetString(mediadb::TITLE, name);
    m_info->SetInteger(mediadb::TYPE, mediadb::QUERY);
}

Query::~Query()
{
}

NodePtr Query::Create(db::Database *thedb, int field, const std::string& name)
{
    return NodePtr(new Query(thedb, field, name));
}

std::string Query::GetName()
{
    if (!m_info)
	GetInfo();
    return m_info->GetString(mediadb::TITLE);
}

bool Query::HasCompoundChildren()
{
    return true;
}

Node::EnumeratorPtr Query::GetChildren()
{
    if (m_children.empty())
    {
	db::QueryPtr qp = m_db->CreateQuery();
	qp->CollateBy(m_field);
	db::RecordsetPtr rs = qp->Execute();

	while (!rs->IsEOF())
	{
	    m_children.push_back(Results::Create(m_db, m_field,
						 rs->GetString(0)));
	    rs->MoveNext();
	}
    }

    return Node::EnumeratorPtr(new ContainerEnumerator<std::vector<NodePtr> >(m_children));
}

db::RecordsetPtr Query::GetInfo()
{
    return m_info;
}

} // namespace mediatree
