#include "tag_editor_widget.h"
#include <QHeaderView>
#include "libutil/trace.h"
#include "libmediadb/schema.h"
#include "libdb/recordset.h"
#include <stdio.h>
#include <set>

namespace choraleqt {

const TagEditorWidget::Column TagEditorWidget::sm_columns[] = {
    { mediadb::ID,             "ID" },
    { mediadb::PATH,           "File" },
    { mediadb::TITLE,          "Title" },
    { mediadb::ARTIST,         "Artist" },
    { mediadb::ALBUM,          "Album" },
    { mediadb::TRACKNUMBER,    "Track" },
    { mediadb::GENRE,          "Genre" },
    { mediadb::YEAR,           "Year" },
    { mediadb::DURATIONMS,     "Duration" },
    { mediadb::MOOD,           "Mood" },
    { mediadb::ORIGINALARTIST, "Original artist" },
    { mediadb::REMIXED,        "Remixed" },
    { mediadb::COMMENT,        "Comment" },
};

const unsigned int TagEditorWidget::NCOLUMNS = (unsigned int)(sizeof(sm_columns)/sizeof(sm_columns[0]));

TagEditorWidget::TagEditorWidget(QWidget *parent, unsigned int flags)
    : QTableWidget(parent),
      m_flags(flags)
{
    setColumnCount((int)NCOLUMNS);

    for (unsigned int i=0; i<NCOLUMNS; ++i)
	setHorizontalHeaderItem(i, new QTableWidgetItem(
				    QString::fromUtf8(sm_columns[i].title)));

    setSortingEnabled(true);
    setAlternatingRowColors(true);

    horizontalHeader()->resizeSection(1, 
				      horizontalHeader()->sectionSize(1)*2);
    horizontalHeader()->setStretchLastSection(true);

    setColumnHidden(0,true);
    if ((flags & SHOW_PATH) == 0)
	setColumnHidden(1, true);
    
    verticalHeader()->hide();

    // Return starts editing
    connect(this, SIGNAL(activated(const QModelIndex&)),
	    this, SLOT(edit(const QModelIndex&)));
}

QColor TagEditorWidget::ColourFor(const QString& s)
{
    if (s.size() == 0)
	return Qt::black;

    if (s.at(0).category() == QChar::Letter_Lowercase)
    {
	TRACE << "Initial lc\n";
	return Qt::red;
    }

    int opens = 0, closes = 0;

    for (unsigned int pos = 0; pos < (unsigned)s.size(); ++pos)
    {
	unsigned int ch = s.at(pos).unicode();

	if (ch == ' ')
	{
	    if (pos+1 == (unsigned)s.size() || s.at(pos+1).unicode() == '.')
	    {
		TRACE << "trailing space or ' .'\n";
		return Qt::red;
	    }
	    if (s.at(pos+1).category() == QChar::Letter_Lowercase)
	    {
		TRACE << "medial lc\n";
		return Qt::red;
	    }
	}
	if (ch == '\'' || ch == '\"')
	{
	    TRACE << "Plain quote\n";
	    return Qt::red;
	}
	if (ch == '(')
	    ++opens;
	if (ch == ')')
	    ++closes;
    }

    if (opens == closes) 
	return Qt::black;

    TRACE << "() mismatch\n";
    return Qt::red;
}

void TagEditorWidget::SetNode(mediatree::NodePtr parent)
{
    if (parent != m_parent)
    {
	m_parent = parent;
	clearContents();
	m_items.clear();
	setSortingEnabled(false);
	
	unsigned int rows = 0;

	std::list<mediatree::NodePtr> items;

	mediatree::Node::EnumeratorPtr ep = parent->GetChildren();
	while (ep->IsValid())
	{
	    mediatree::NodePtr np = ep->Get();

	    ++rows;
	    setRowCount(rows);
	    setRowHeight(rows-1, (QFontInfo(font()).pixelSize() * 5) /4 + 4);

	    items.push_back(np);

	    util::CountedPointer<db::Recordset> rs = np->GetInfo();
	    
	    bool is_media = !np->IsCompound();

	    for (unsigned int i=0; i<NCOLUMNS; ++i)
	    {
		QTableWidgetItem *item = new QTableWidgetItem;

		if (i == 0)
		{
		    /* Store the row number in the item (for column 0) so we
		     * can work out which item this row corresponds to even if
		     * the user re-sorts the table.
		     *
		     * NB row id counts from 1 not 0, to avoid false positives
		     * for "row 0" in unset data
		     */
		    item->setData(Qt::UserRole, QVariant((uint)rows));
		}

		if (sm_columns[i].field == mediadb::DURATIONMS)
		{
		    unsigned int durationms
			= rs->GetInteger(mediadb::DURATIONMS);
		    char tc[80];

		    unsigned int durations = (durationms+999) / 1000;

		    if (durations > 3600) // 1 hour
			sprintf(tc, "%u:%02u:%02u", (durations/3600),
				(durations/60)%60, durations%60);
		    else
			sprintf(tc, "%u:%02u", durations/60, durations%60);
		    item->setText(QString::fromUtf8(tc));

		    item->setFlags(Qt::ItemIsEnabled); // i.e. not editable
		    item->setTextAlignment(Qt::AlignRight);
		}
		else
		{
		    QString s(QString::fromUtf8(
				  rs->GetString(sm_columns[i].field).c_str()));
		    
		    item->setText(s);
		    if (!is_media)
			item->setFlags(Qt::ItemIsEnabled); // i.e. not editable

		    if (sm_columns[i].field == mediadb::ARTIST
			|| sm_columns[i].field == mediadb::ALBUM
			|| sm_columns[i].field == mediadb::TITLE)
			item->setForeground(ColourFor(s));
		}

		setItem(rows-1, i, item);
	    }
	    ep->Next();
	}

	m_items.resize(items.size());
	std::copy(items.begin(), items.end(), m_items.begin());

	resizeColumnsToContents();
	setSortingEnabled(true);
    }
}

void TagEditorWidget::commitData(QWidget *editor)
{
    QTableWidgetItem *changed_item = currentItem();

    QString old_text = changed_item->text();

    QTableWidget::commitData(editor);

    QString new_text = changed_item->text();

    if (old_text != new_text)
    {
	QColor colour = ColourFor(new_text);
	changed_item->setForeground(colour);

	std::set<unsigned int> rewrite_rows;
	rewrite_rows.insert(changed_item->row());

	QList<QTableWidgetItem*> twil = selectedItems();
    
	for (int i=0; i<twil.size(); ++i)
	{
	    QTableWidgetItem *twi = twil[i];
	    if (twi != changed_item && (twi->flags() & Qt::ItemIsEditable))
	    {
		if (twi->text() != new_text)
		{
		    twi->setText(new_text);
		    twi->setForeground(colour);

		    rewrite_rows.insert(twi->row());
		}
	    }
	}

	for (std::set<unsigned int>::const_iterator i = rewrite_rows.begin();
	     i != rewrite_rows.end();
	     ++i)
	{
	    unsigned int row = *i;

	    // What we really want is "row = logicalRow(row)" but
	    // there isn't one
	    unsigned int logicalRow = item(row,0)->data(Qt::UserRole).toUInt()-1;

	    if (logicalRow >= m_items.size())
	    {
		TRACE << "Bogus row " << row << "/" << m_items.size() << "\n";
		continue;
	    }

	    util::CountedPointer<db::Recordset> rs = m_items[logicalRow]->GetInfo();
	    if (!rs || rs->IsEOF())
	    {
		TRACE << "No rs for node\n";
		continue;
	    }

	    // Check we got the right row
	    unsigned int id = atoi(item(row, 0)->text().toUtf8());
	    if (id != rs->GetInteger(mediadb::ID))
	    {
		TRACE << "id=" << id << " not "
		      << rs->GetInteger(mediadb::ID) << "\n";
		continue;
	    }
	   
	    for (unsigned int column = 0;
		 column < NCOLUMNS;
		 ++column)
	    {
		unsigned int field = sm_columns[column].field;
		if (field != mediadb::ID
		    && field != mediadb::PATH
		    && field != mediadb::SIZEBYTES
		    && field != mediadb::DURATIONMS)
		{
		    std::string newtext(item(row, column)->text().toUtf8().data());
//		    TRACE << "Set field " << field << " to '" << newtext
//			  << "'\n";
		    rs->SetString(field, newtext);
		}
	    }

	    unsigned int rc = rs->Commit();
	    if (rc)
	    {
		TRACE << "Can't commit data: " << rc << "\n";
	    }
	}
    }
}

} // namespace choraleqt
