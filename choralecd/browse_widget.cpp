/* browse_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "browse_widget.h"
#include "libutil/trace.h"
#include "libmediadb/schema.h"
#include <qcursor.h>
#include <q3cstring.h>
#include <QMouseEvent>
#include <QDrag>

namespace choraleqt {

BrowseWidget::BrowseWidget(QWidget *parent, unsigned int dbid)
    : QListWidget(parent),
      m_dbid(dbid)
{
    setResizeMode(Adjust);
    setSelectionMode(ExtendedSelection);
    setWordWrap(false);
    setSpacing(0);
    setViewMode(ListMode);
    setWrapping(true);
    setDragDropMode(DragOnly);
}

void BrowseWidget::SetNode(mediatree::NodePtr np)
{
    if (np != m_node)
    {
	m_node = np;
	clear();
	
	mediatree::Node::EnumeratorPtr ep = m_node->GetChildren();
	while (ep->IsValid())
	{
	    new BrowseItem(this, 
			   QString::fromUtf8(ep->Get()->GetName().c_str()),
			   ep->Get());
	    ep->Next();
	}
    }
}

QStringList BrowseWidget::mimeTypes() const
{
    QStringList qsl;
    qsl << "application/x-chorale-ids";
    return qsl;
}

/** Drag-and-drop.
 *
 * Here's the plan. We send a vector of (mediadb-id, fid) pairs. Receiving
 * end looks them up in the registry (libmediadb/registry.h) and can thus
 * convert them to URLs. (Perhaps we also pass them as text/uri-list for DnD
 * to non-Chorale apps.)
 */
QMimeData *BrowseWidget::mimeData(const QList<QListWidgetItem*> items) const
{
    std::vector<IDPair> idpv;
    idpv.resize(items.size());

    for (unsigned int i=0; i<(unsigned)items.size(); ++i)
    {
	BrowseItem *browse_item = (BrowseItem*)items[i];
	idpv[i].dbid = m_dbid;
	idpv[i].fid = browse_item->GetNode()->GetInfo()->GetInteger(mediadb::ID);
    }
    
    QByteArray a;
    a.resize(idpv.size() * sizeof(IDPair));
    memcpy(a.data(), &idpv[0], idpv.size() * sizeof(IDPair));

    QMimeData *md = new QMimeData;
    md->setData("application/x-chorale-ids", a);
    return md;
}

} // namespace choraleqt
