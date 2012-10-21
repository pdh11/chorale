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

} // namespace choraleqt
