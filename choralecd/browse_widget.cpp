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

namespace choraleqt {

BrowseWidget::BrowseWidget(QWidget *parent, unsigned int dbid)
    : Q3IconView(parent),
      m_dbid(dbid)
{
    setAutoArrange(true);
    setResizeMode(Adjust);
    setSelectionMode(Extended);
    setItemTextPos(Right);
    setItemsMovable(false);
    setWordWrapIconText(false);
    setSpacing(0);
    setArrangement(TopToBottom);
    setMaxItemWidth(400);
}

void BrowseWidget::SetNode(mediatree::NodePtr np)
{
    if (np != m_node)
    {
	m_node = np;
	clear();

	TRACE << "New node is '" << m_node->GetName() << "'\n";
	
	mediatree::Node::EnumeratorPtr ep = m_node->GetChildren();
	while (ep->IsValid())
	{
	    TRACE << "And child.\n";
	    new BrowseItem(this, 
			   QString::fromUtf8(ep->Get()->GetName().c_str()),
			   QPixmap(),
			   ep->Get());
	    ep->Next();
	}
    }
}

/** Sheer black magic here from Qt example "fileiconview", qv.
 */
Q3DragObject* BrowseWidget::dragObject()
{
    if ( !currentItem() )
	return 0;

    QPoint orig = viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );
    BrowseIconDrag *drag = new BrowseIconDrag( viewport() );
    drag->setPixmap( *currentItem()->pixmap(),
		     QPoint( currentItem()->pixmapRect().width() / 2, 
			     currentItem()->pixmapRect().height() / 2 ) );

    for (BrowseItem *item = (BrowseItem*)firstItem();
	 item;
	 item = (BrowseItem*)item->nextItem() ) 
    {
	if ( item->isSelected() ) 
	{
	    Q3IconDragItem id;
	    id.setData( Q3CString("hello there") );
	    drag->append( id,
			  QRect( item->pixmapRect( FALSE ).x() - orig.x(),
				 item->pixmapRect( FALSE ).y() - orig.y(),
				 item->pixmapRect().width(), item->pixmapRect().height() ),
			  QRect( item->textRect( FALSE ).x() - orig.x(),
				 item->textRect( FALSE ).y() - orig.y(),
				 item->textRect().width(), item->textRect().height() ),
			  m_dbid,
			  item->GetNode()->GetInfo()->GetInteger(mediadb::ID)
		);
	}
    }

    return drag;
}

/** Drag-and-drop.
 *
 * Here's the plan. We send a vector of (mediadb-id, fid) pairs. Receiving
 * end looks them up in the registry (libmediadb/registry.h) and can thus
 * convert them to URLs. (Perhaps we also pass them as text/uri-list for DnD
 * to non-Chorale apps.)
 */

BrowseIconDrag::BrowseIconDrag(QWidget *dragSource)
    : Q3IconDrag(dragSource, NULL)
{
}

const char* BrowseIconDrag::format( int i ) const
{
    if ( i == 0 )
        return "application/x-qiconlist";
    else if ( i == 1 )
        return "application/x-chorale-ids";
    else
        return 0;
}

QByteArray BrowseIconDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    if ( QString( mime ) == "application/x-qiconlist" )
    {
        a = Q3IconDrag::encodedData( mime );
    }
    else if ( QString( mime ) == "application/x-chorale-ids" )
    {
	a.resize(m_pairs.size() * sizeof(IDPair));
	memcpy(a.data(), &m_pairs[0], m_pairs.size() * sizeof(IDPair));
    }
    return a;
}

bool BrowseIconDrag::canDecode( QMimeSource* e )
{
    return e->provides("application/x-qiconlist")
	|| e->provides("application/x-chorale-ids");
}

void BrowseIconDrag::append(const Q3IconDragItem &item, const QRect &pr,
			    const QRect &tr, unsigned int dbid, 
			    unsigned int fid)
{
    Q3IconDrag::append( item, pr, tr );

    IDPair idp;
    idp.dbid = dbid;
    idp.fid = fid;
    m_pairs.push_back(idp);
}

};
