#include "root.h"
#include "query.h"
#include "directory.h"
#include "node_enumerator.h"
#include "libdb/free_rs.h"
#include "libdb/query.h"
#include "libmediadb/schema.h"

namespace mediatree {

Root::Root(db::Database *thedb, uint32_t flags)
    : m_db(thedb)
{
    if (flags & (1<<mediadb::ALBUM))
	m_children.push_back(Query::Create(thedb, mediadb::ALBUM, "Albums"));
    if (flags & (1<<mediadb::ARTIST))
	m_children.push_back(Query::Create(thedb, mediadb::ARTIST, "Artists"));
    if (flags & (1<<mediadb::GENRE))
	m_children.push_back(Query::Create(thedb, mediadb::GENRE, "Genres"));
    m_children.push_back(Directory::Create(thedb, 0x100));
}

Root::~Root()
{
}

NodePtr Root::Create(db::Database *thedb)
{
    uint32_t flags = 0;

    db::QueryPtr qp = thedb->CreateQuery();
    if (qp->CollateBy(mediadb::ALBUM) == 0) // i.e., is this query allowed?
	flags |= (1<<mediadb::ALBUM);
    qp = thedb->CreateQuery();
    if (qp->CollateBy(mediadb::ARTIST) == 0)
	flags |= (1<<mediadb::ARTIST);
    qp = thedb->CreateQuery();
    if (qp->CollateBy(mediadb::GENRE) == 0)
	flags |= (1<<mediadb::GENRE);

    if (flags == 0)
    {
	TRACE << "No soup views, starting with plain folder root\n";
	return Directory::Create(thedb, 0x100);
    }

    return NodePtr(new Root(thedb, flags));
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

} // namespace mediatree
