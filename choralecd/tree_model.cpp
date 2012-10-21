/* tree_model.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "tree_model.h"
#include "libmediatree/root.h"
#include "libutil/trace.h"
#include "libutil/counted_pointer.h"
#include "libdb/recordset.h"
#include "libmediadb/schema.h"

#ifndef HAVE_DIR_XPM
#include "imagery/dir.xpm"
#define HAVE_DIR_XPM
#endif
#include "imagery/query.xpm"

namespace choraleqt {

/** We don't use NodePtrs directly, to avoid circular reference problems.
 */
class TreeModel::Item
{
public:
    Item(Item *p, mediatree::NodePtr n) 
	: parent(p), node(n), got_children(false) {}
    ~Item();

    Item *parent;
    mediatree::NodePtr node;
    bool got_children;
    std::vector<Item*> children;

    Item *GetChild(unsigned int n);
    unsigned int GetRow() const;
};

TreeModel::Item::~Item()
{
    for (std::vector<Item*>::iterator i = children.begin();
	 i != children.end();
	 ++i)
	delete *i;
}

TreeModel::Item *TreeModel::Item::GetChild(unsigned int n)
{
    if (!got_children)
    {
	mediatree::Node::EnumeratorPtr ep = node->GetChildren();
	while (ep->IsValid())
	{
	    mediatree::NodePtr n = ep->Get();
	    if (n->IsCompound())
		children.push_back(new Item(this, n));
	    ep->Next();
	}
	got_children = true;
	TRACE << children.size() << " children\n";
    }
    if (n >= children.size())
	return NULL;
    return children[n];
}

unsigned int TreeModel::Item::GetRow() const
{
    if (!parent)
	return 0;
    for (unsigned i=0; i<parent->children.size(); ++i)
	if (parent->children[i] == this)
	    return i;
    return 0;
}

TreeModel::TreeModel(db::Database *db)
    : m_root(new Item(NULL, mediatree::Root::Create(db))),
      m_dir_pixmap(dir_xpm),
      m_query_pixmap(query_xpm)
{
}

TreeModel::~TreeModel()
{
    delete m_root;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
//    TRACE << "index(" << row << "," << column << ")\n";

    if (!hasIndex(row, column, parent))
	return QModelIndex();

    Item *parentItem;

    if (!parent.isValid())
	parentItem = m_root;
    else
	parentItem = (Item*)parent.internalPointer();

    Item *childItem = parentItem->GetChild(row);
    if (childItem)
	return createIndex(row, column, childItem);
    else
	return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
//    TRACE << "parent()\n";
    if (!index.isValid())
	return QModelIndex();

    Item *childItem = (Item*)index.internalPointer();
    Item *parentItem = childItem->parent;

     if (parentItem == m_root || !parentItem)
         return QModelIndex();

     return createIndex(parentItem->GetRow(), 0, parentItem);
}

mediatree::NodePtr TreeModel::NodeForIndex(const QModelIndex& node) 
{
    if (node.column() > 0)
	return mediatree::NodePtr();

    Item *nodeItem;
    if (!node.isValid())
	nodeItem = m_root;
    else
	nodeItem = (Item*)node.internalPointer();

    return nodeItem->node;
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
//    TRACE << "rowCount()\n";
    if (parent.column() > 0)
	return 0;

    Item *parentItem;
    if (!parent.isValid())
	parentItem = m_root;
    else
	parentItem = (Item*)parent.internalPointer();

    parentItem->GetChild(0);

//    TRACE << parentItem->children.size() << "\n";
    return (int)parentItem->children.size();
}

bool TreeModel::hasChildren(const QModelIndex& parent) const
{
//    TRACE << "hasChildren()\n";
    if (parent.column() > 0)
	return false;

    Item *parentItem;
    if (!parent.isValid())
	parentItem = m_root;
    else
	parentItem = (Item*)parent.internalPointer();

    if (parentItem->got_children)
	return !parentItem->children.empty();

    /* In the mediatree model, only items with children should be even
     * visible in the tree-view. So for a tree-view item to have
     * children, its corresponding node must have grandchildren.
     */
    return parentItem->node->HasCompoundChildren();
}

int TreeModel::columnCount(const QModelIndex&) const
{
//    TRACE << "columnCount()\n";
    return 1;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
	return QVariant();

    if (role != Qt::DisplayRole
	&& role != Qt::DecorationRole)
	return QVariant();

    Item *item = (Item*)index.internalPointer();

    if (role == Qt::DecorationRole)
    {
	unsigned int type = item->node->GetInfo()->GetInteger(mediadb::TYPE);
	if (type == mediadb::DIR || type == mediadb::PLAYLIST)
	    return QVariant(m_dir_pixmap);
	if (type == mediadb::QUERY)
	    return QVariant(m_query_pixmap);
	return QVariant();
    }

    return QVariant(QString::fromUtf8(item->node->GetName().c_str()));
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
	return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int, Qt::Orientation, int) const
{
    return QVariant();
}



} // namespace choraleqt
