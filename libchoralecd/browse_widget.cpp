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
#include "libdb/recordset.h"
#include <qcursor.h>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

#ifndef HAVE_DIR_XPM
#include "imagery/dir.xpm"
#define HAVE_DIR_XPM
#endif
#include "imagery/file.xpm"
#include "imagery/film.xpm"
#include "imagery/image.xpm"

namespace choraleqt {

BrowseWidget::BrowseWidget(QWidget *parent, unsigned int dbid,
			   QPixmap *dir_pixmap, QPixmap *file_pixmap)
    : QListWidget(parent),
      m_dbid(dbid),
      m_dir_pixmap(dir_pixmap),
      m_file_pixmap(file_pixmap)
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
	    mediatree::NodePtr np2 = ep->Get();
	    BrowseItem *item = new BrowseItem(this, 
					      QString::fromUtf8(np->GetName().c_str()),
					      np2);
	    if (np2->IsCompound())
	    {
		item->setIcon(QPixmap(dir_xpm));
	    }
	    else
	    {
		unsigned int type = np2->GetInfo()->GetInteger(mediadb::TYPE);
		switch (type)
		{
		case mediadb::IMAGE:
		    item->setIcon(QPixmap(image_xpm));
		    break;
		case mediadb::VIDEO:
		case mediadb::TV:
		    item->setIcon(QPixmap(film_xpm));
		    break;
		default:
		    item->setIcon(QPixmap(file_xpm));
		    break;
		}
	    }
	    ep->Next();
	}
    }
}

QStringList BrowseWidget::mimeTypes() const
{
    QStringList qsl;
    qsl << QString::fromUtf8("application/x-chorale-ids");
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
    a.resize((int)(idpv.size() * sizeof(IDPair)));
    memcpy(a.data(), &idpv[0], idpv.size() * sizeof(IDPair));

    QMimeData *md = new QMimeData;
    md->setData(QString::fromUtf8("application/x-chorale-ids"), a);
    return md;
}

} // namespace choraleqt
