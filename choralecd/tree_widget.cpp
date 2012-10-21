/* tree_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "tree_widget.h"
#include "libmediadb/schema.h"
#include "libutil/trace.h"

namespace choraleqt {

void NodeItem::setOpen(bool whether)
{
    if (whether && !childCount())
    {
	mediatree::Node::EnumeratorPtr ep = m_node->GetChildren();
	while (ep->IsValid())
	{
	    new NodeItem(this, ep->Get());
	    ep->Next();
	}

	if (!childCount())
	    setExpandable(false);
    }
    Q3ListViewItem::setOpen(whether);
}

void NodeItem::setup()
{
    setExpandable(m_node->IsCompound());
    Q3ListViewItem::setup();
}

QString NodeItem::text(int) const
{
    return QString::fromUtf8(
	m_node->GetInfo()->GetString(mediadb::TITLE).c_str());
}

TreeWidget::TreeWidget(QWidget *parent, mediatree::NodePtr root)
    : Q3ListView(parent),
      m_root(root)
{
    setRootIsDecorated(true);
    addColumn("Folders");
    mediatree::Node::EnumeratorPtr ep = root->GetChildren();
    while (ep->IsValid())
    {
	(void)new NodeItem(this, ep->Get());
	ep->Next();
    }
}

};
