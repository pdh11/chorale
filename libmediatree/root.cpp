#include "root.h"
#include "query.h"
#include "directory.h"
#include "node_enumerator.h"
#include "libdb/free_rs.h"
#include "libmediadb/schema.h"

namespace mediatree {

Root::Root(db::Database *thedb)
    : m_db(thedb)
{
    m_children.push_back(Query::Create(thedb, mediadb::ARTIST, "Artists"));
    m_children.push_back(Query::Create(thedb, mediadb::GENRE, "Genres"));
    m_children.push_back(Directory::Create(thedb, 0x100));
}

NodePtr Root::Create(db::Database *thedb)
{
    return NodePtr(new Root(thedb));
}

std::string Root::GetName()
{
    return "media";
}

bool Root::HasCompoundChildren()
{
    return true;
}

Node::EnumeratorPtr Root::GetChildren()
{
    TRACE << "Getting children of root\n";
    return Node::EnumeratorPtr(new ContainerEnumerator<std::vector<NodePtr> >(m_children));
}

db::RecordsetPtr Root::GetInfo()
{
    return db::FreeRecordset::Create();
}

}; // namespace mediatree
