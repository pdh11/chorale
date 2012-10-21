/* tagtable.h */

#ifndef TAGTABLE_H
#define TAGTABLE_H

#include <q3iconview.h>
#include <q3table.h>
#include <QPainter>
#include <QColorGroup>
#include <set>

namespace choraleqt {

/** Override the two size-changed slots to do something more sensible if
 * window isn't shown.
 */
class TagTable: public Q3Table
{
    Q_OBJECT;

 public:
    TagTable(int rows, int columns, QWidget *parent);

    virtual void columnWidthChanged(int which);
    virtual void rowHeightChanged(int which);
    virtual QWidget *beginEdit(int,int,bool);
    virtual void keyPressEvent( QKeyEvent* );

 protected:
    // Must reimplement these as they are private :-(
    QSize tableSize2() const;
    void updateGeometries2();

 protected:
    void checkUpdate( int row, int col, QString text,
		      std::set<unsigned int>* needs_updating );
//    void buildIndexMap();
    void doAutoFixup();

    int m_lastSortCol;
    bool m_sortAsc;
    QString m_oldText; // so we can tell whether a valueChanged is real or not
};

/** A table item that repaints in a given colour */
class ColouredItem: public Q3TableItem
{
    QColor m_colour;

public:
    ColouredItem( Q3Table *table, const QString& text, QColor col )
	: Q3TableItem( table, OnTyping, text ), m_colour(col)
	{
	    setReplaceable(false);
	}

    void SetColour(QColor col) { m_colour = col; }

    virtual void paint( QPainter *p, const QColorGroup& cg, const QRect& cr,
			bool selected )
	{
	    QColorGroup cg2 = cg;
	    if (m_colour.isValid())
		cg2.setColor( QColorGroup::Text, m_colour );
	    if (row()%2)
		cg2.setColor( QColorGroup::Base, QColor(0xCC,0xFF,0xFF) );
	    Q3TableItem::paint( p, cg2, cr, selected );
	}
    virtual int alignment() const { return Qt::AlignLeft | Qt::AlignVCenter; }
};

/** A table item that sorts numerically, not lexicographically, and displays
 * zero as blank.
 */
class NumericTableItem: public ColouredItem
{
public:
    NumericTableItem( Q3Table *table,
		      Q3TableItem::EditType,
		      const QString& text )
	: ColouredItem( table, text, Qt::black ) {}
    QString key() const;
    QString text() const;
    virtual int alignment() const { return Qt::AlignRight | Qt::AlignVCenter; }
};


/** A table item that sorts numerically, but doesn't display the actual number
 * (e.g. for time_t's). Probably makes no sense to be editable.
 */
class HiddenNumericTableItem: public ColouredItem
{
    unsigned int m_value;

public:
    HiddenNumericTableItem( Q3Table *table,
			    Q3TableItem::EditType et,
			    const QString& text,
			    unsigned int value );
    QString key() const;
    virtual int alignment() const { return Qt::AlignRight | Qt::AlignVCenter; }
};

class BoldableTableItem: public Q3TableItem
{
    bool m_bold;
public:
    BoldableTableItem( Q3Table *table, const QString& text)
	: Q3TableItem( table, OnTyping, text ),
	  m_bold(false)
    {
    }

    void SetBold(bool whether) { m_bold = whether; }

    virtual void paint(QPainter *p, const QColorGroup& cg, const QRect& cr,
		       bool selected)
    {
	QFont font = p->font();
	if (m_bold)
	{
	    font.setBold(true);
	    p->setFont(font);
	}
	QColorGroup cg2 = cg;
	if (row()%2)
	    cg2.setColor(QColorGroup::Base, QColor(0xCC,0xFF,0xFF));
	Q3TableItem::paint(p, cg2, cr, selected);
    }
};

}; // namespace choraleqt

#endif
