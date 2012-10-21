/* tagtable.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "tagtable.h"
#include <QColorGroup>
#include <QKeyEvent>
#include <set>

namespace choraleqt {

TagTable::TagTable(int rows, int columns, QWidget *parent)
	: Q3Table(rows,columns,parent) 
{
}

QSize TagTable::tableSize2() const
{
    return QSize( columnPos( numCols() - 1 ) + columnWidth( numCols() - 1 ),
		  rowPos( numRows() - 1 ) + rowHeight( numRows() - 1 ) );
}

void TagTable::updateGeometries2()
{
    QSize ts = tableSize2();
    if ( horizontalHeader()->offset() &&
	 ts.width() < horizontalHeader()->offset() + horizontalHeader()->width() )
	horizontalScrollBar()->setValue( ts.width() - horizontalHeader()->width() );
    if ( verticalHeader()->offset() &&
	 ts.height() < verticalHeader()->offset() + verticalHeader()->height() )
	verticalScrollBar()->setValue( ts.height() - verticalHeader()->height() );

    verticalHeader()->setGeometry( frameWidth(), topMargin() + frameWidth(),
			     leftMargin(), visibleHeight() );
    horizontalHeader()->setGeometry( leftMargin() + frameWidth(), frameWidth(),
			    visibleWidth(), topMargin() );
//    inUpdateGeometries = FALSE;
}

void TagTable::columnWidthChanged( int col )
{
    if (isEnabled() && isVisible() && isUpdatesEnabled())
    {
	Q3Table::columnWidthChanged(col);
	return;
    }

    // Otherwise we can do better
    updateContents( columnPos( col ), 0, contentsWidth(), contentsHeight() );
    QSize s( tableSize2() );
    resizeContents( s.width(), s.height() );

    updateGeometries2();

    for ( int j = col; j < numCols(); ++j ) {
	for ( int i = 0; i < numRows(); ++i ) {
	    QWidget *w = cellWidget( i, j );
	    if ( !w )
		continue;
	    moveChild( w, columnPos( j ), rowPos( i ) );
	    w->resize( columnWidth( j ) - 1, rowHeight( i ) - 1 );
	}
    }
}

void TagTable::rowHeightChanged( int row )
{
    if (isEnabled() && isVisible() && isUpdatesEnabled())
    {
	Q3Table::rowHeightChanged(row);
	return;
    }

    // Otherwise we can do better
    updateContents( 0, rowPos( row ), contentsWidth(), contentsHeight() );
    QSize s( tableSize2() );
    resizeContents( s.width(), s.height() );

    updateGeometries2();

    for ( int j = row; j < numRows(); ++j ) {
	for ( int i = 0; i < numCols(); ++i ) {
	    QWidget *w = cellWidget( j, i );
	    if ( !w )
		continue;
	    moveChild( w, columnPos( i ), rowPos( j ) );
	    w->resize( columnWidth( i ) - 1, rowHeight( j ) - 1 );
	}
    }
}

QWidget *TagTable::beginEdit( int row, int col, bool replace )
{
    m_oldText = text(row,col);

    return Q3Table::beginEdit( row, col, replace );
}

/** Return starts in-place editing (why isn't this the default?) */
void TagTable::keyPressEvent( QKeyEvent *e )
{
    if ( e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter )
    {
	// Fake an F2, which does start editing
	QKeyEvent e2( QKeyEvent::KeyPress, Qt::Key_F2, 0, e->state() );
	Q3Table::keyPressEvent(&e2);
    }
    else
	Q3Table::keyPressEvent(e);
}


/* Table item implementations */

QString NumericTableItem::key() const
{
    QString s = "0000000000" + text();
    return s.right(10);
}

QString NumericTableItem::text() const
{
    QString s = Q3TableItem::text();
    if (s == "0")
	return "";
    return s;
}

HiddenNumericTableItem::HiddenNumericTableItem( Q3Table *table,
						Q3TableItem::EditType,
						const QString& text,
						unsigned int value )
    : ColouredItem( table, text, Qt::black ), m_value(value)
{
}

QString HiddenNumericTableItem::key() const
{
    QString s;
    s.sprintf( "%010d", m_value );
    return s;
}

};
