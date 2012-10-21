#include "directory.h"
#include "node_enumerator.h"
#include "libdb/db.h"
#include "libmediadb/schema.h"
#include "libutil/trace.h"

namespace mediatree {

Directory::Directory(db::Database *thedb, unsigned int id)
    : m_db(thedb),
      m_id(id)
{
}

Directory::Directory(db::Database *thedb, unsigned int id, 
		     db::RecordsetPtr info)
    : m_db(thedb),
      m_id(id),
      m_info(info)
{
}

NodePtr Directory::Create(db::Database *thedb, unsigned int id)
{
    return NodePtr(new Directory(thedb, id));
}

NodePtr Directory::Create(db::Database *thedb, unsigned int id, 
			  db::RecordsetPtr info)
{
    return NodePtr(new Directory(thedb, id, info));
}

std::string Directory::GetName()
{
    if (!m_info)
	GetInfo();
    return m_info->GetString(mediadb::TITLE);
}

bool Directory::IsCompound()
{
    if (!m_info)
	GetInfo();
    unsigned int type = m_info->GetInteger(mediadb::TYPE);
    return type == mediadb::DIR || type == mediadb::PLAYLIST;
}

bool Directory::HasCompoundChildren()
{
    return true;
}

Node::EnumeratorPtr Directory::GetChildren()
{
    if (m_children.empty())
    {
	GetInfo();
	std::vector<unsigned int> vec;
	mediadb::ChildrenToVector(m_info->GetString(mediadb::CHILDREN), &vec);
	m_children.resize(vec.size());
	for (unsigned int i=0; i<vec.size(); ++i)
	    m_children[i] = NodePtr(new Directory(m_db, vec[i]));
    }

    return Node::EnumeratorPtr(new ContainerEnumerator<std::vector<NodePtr> >(m_children));
}

db::RecordsetPtr Directory::GetInfo()
{
    if (!m_info)
    {
	TRACE << "Getting info for id " << m_id << "\n";
	db::QueryPtr qp = m_db->CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, m_id));
	m_info = qp->Execute();
    }
    return m_info;
}

} // namespace mediatree
