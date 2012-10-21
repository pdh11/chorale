/* cd_window.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "cd_window.h"
#include "features.h"
#include "libutil/trace.h"
#include <qradiobutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <QMenu>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <qlineedit.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qcursor.h>
#include <qstring.h>
#include "events.h"
#include "settings.h"
#include <sstream>
#include <iomanip>
#include "libmediadb/schema.h"
#include "libdb/recordset.h"
#include <fcntl.h>
#include <stdio.h>

#if HAVE_CD

namespace choraleqt {

class ImmutableTableItem: public QTableWidgetItem
{
public:
    explicit ImmutableTableItem(const char *s)
	: QTableWidgetItem(s)
	{
	    setFlags(Qt::ItemIsEnabled);
	}
};

struct CDWindow::Entry
{
    unsigned rt_percent;
    unsigned et1_percent;
    unsigned et2_percent;
    /// @todo bool cancelled
};

static const struct
{
    unsigned int field;
    const char *title;
} columns[] = {
    { mediadb::TITLE,          "Title" },
    { mediadb::ARTIST,         "Artist" },
    { mediadb::GENRE,          "Genre" },
    { mediadb::YEAR,           "Year" },
    { mediadb::DURATIONMS,     "Duration" },
    { 0,                       "Progress" },
    { mediadb::MOOD,           "Mood" },
    { mediadb::ORIGINALARTIST, "Original artist" },
    { mediadb::REMIXED,        "Remixed" },
    { mediadb::COMMENT,        "Comment" }
};

enum { NUM_COLUMNS = sizeof(columns)/sizeof(columns[0]) };

CDWindow::CDWindow(import::CDDrivePtr drive, import::AudioCDPtr cd,
		   import::CDDBLookupPtr cddb,
		   const Settings *settings, util::TaskQueue *cpu_queue,
		   util::TaskQueue *disk_queue)
    : QDialog(NULL, NULL),
      m_rip(drive, cd, cpu_queue, disk_queue,
	    settings->GetMP3Root(), settings->GetFlacRoot()),
      m_ntracks((unsigned int)cd->GetTrackCount()),
      m_entries(new Entry[m_ntracks]),
      m_cdtype(NULL),
      m_albumname(NULL),
      m_trackoffset(NULL),
      m_table(NULL),
      m_progress_column(0),
      m_cddb(cddb)
{
    m_rip.AddObserver(this);

    unsigned total_sectors = cd->GetTotalSectors();

    std::ostringstream caption;
    caption << "CD in " << drive->GetName()
	    << " (" << (total_sectors / (75*60)) << ":"
	    << std::setw(2) << std::setfill('0')
	    << ((total_sectors/75) % 60) << ")";

    setWindowTitle(QString::fromUtf8(caption.str().c_str()));

    QVBoxLayout *vlayout = new QVBoxLayout(this);//, 0, 6);

    QHBoxLayout *toplayout = new QHBoxLayout();//NULL, 0, 6);

    QRadioButton *rb1 = new QRadioButton("&Single-artist", this);
    rb1->setChecked(true);
    toplayout->addWidget(rb1);

    QRadioButton *rb2 = new QRadioButton("C&ompilation", this);
    toplayout->addWidget(rb2);

    QRadioButton *rb3 = new QRadioButton("&Mixed", this);
    toplayout->addWidget(rb3);

    m_cdtype = new QButtonGroup(this);
    m_cdtype->addButton(rb1, SINGLE_ARTIST);
    m_cdtype->addButton(rb2, COMPILATION);
    m_cdtype->addButton(rb3, MIXED);

    toplayout->addStretch(10);

    toplayout->addWidget(new QLabel("Album", this));
    m_albumname = new QLineEdit("Album 1", this);
    toplayout->addWidget(m_albumname, 10);

    toplayout->addStretch(10);

    toplayout->addWidget(new QLabel("Track offset", this));

    m_trackoffset = new QSpinBox(this);
    m_trackoffset->setRange(0,99);
    toplayout->addWidget(m_trackoffset);
    toplayout->addStretch(10);

    if (cddb)
    {
	QPushButton *cddbalt = new QPushButton("&CDDB...", this);
	toplayout->addWidget(cddbalt);
	toplayout->addStretch(1);
	connect(cddbalt, SIGNAL(clicked()), this, SLOT(OnCDDB()));
    }

    vlayout->addLayout(toplayout);

    m_table = new QTableWidget(m_ntracks, (sizeof(columns)/sizeof(*columns)),
			       this);
    vlayout->addWidget(m_table, 1);

    int duration_column = 0;

    for (unsigned int i=0; i<(sizeof(columns)/sizeof(*columns)); ++i)
    {
	m_table->setHorizontalHeaderItem(i,
					 new QTableWidgetItem(columns[i].title));
//	if (columns[i].field == 0 || columns[i].field == mediadb::DURATIONMS)
//	    m_table->setColumnReadOnly(i, true);
	if (columns[i].field == 0)
	    m_progress_column = i;
	if (columns[i].field == mediadb::DURATIONMS)
	    duration_column = i;
    }

    m_table->setSortingEnabled(false);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->resizeSection(0, 
					       m_table->horizontalHeader()->sectionSize(0)*2);
    m_table->horizontalHeader()->setStretchLastSection(true);

    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	unsigned start = (cd->begin()+i)->first_sector;
	unsigned end   = (cd->begin()+i)->last_sector;
	unsigned sectors = end-start+1;
	std::ostringstream os;
	os << (sectors / (75*60)) << ":"
	   << std::setw(2) << std::setfill('0')
	   << ((sectors / 75) % 60) << "."
	   << std::setw(2) << std::setfill('0')
	   << (sectors % 75);
	m_table->setItem(i, duration_column,
			 new ImmutableTableItem(os.str().c_str()));
	m_table->setItem(i, m_progress_column, new ImmutableTableItem(""));

	for (unsigned int j=0; j<sizeof(columns)/sizeof(*columns); ++j)
	{
	    if (j != m_progress_column && j != (unsigned)duration_column)
		m_table->setItem(i, j, new QTableWidgetItem);
	}
	
	m_table->setRowHeight(i, (QFontInfo(font()).pixelSize() * 5) /4 + 4);
    }

    if (cddb)
	SetUpCDDBStrings(0);

    connect(m_table, SIGNAL(itemChanged(QTableWidgetItem*)),
	    this, SLOT(OnTableItemChanged(QTableWidgetItem*)));

    QHBoxLayout *blayout = new QHBoxLayout;
    blayout->addStretch(1);

//    QPushButton *cancel = new QPushButton("&Cancel", this);
//    cancel->setAutoDefault(false);
//    blayout->addWidget(cancel);
    QPushButton *done = new QPushButton("&Done", this);
    done->setAutoDefault(false);
    blayout->addWidget(done);
    blayout->addSpacing(20); // clear of the resize handle

    vlayout->addLayout(blayout);
    vlayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    setSizeGripEnabled(true);

    connect(done, SIGNAL(clicked()), this, SLOT(OnDone()));

    show();

    size_t newheight = maximumHeight();
    size_t screenheight = QApplication::desktop()->height();

    if (newheight > (screenheight*7/8))
	newheight = screenheight *7/8;
    resize((int)newheight, width());
}

CDWindow::~CDWindow()
{
    TRACE << "~CDWindow\n";
    QApplication::removePostedEvents(this);
    delete[] m_entries;
}

static QColor ColourFor(const QString& s)
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

void CDWindow::OnTableItemChanged(QTableWidgetItem *item)
{
    if (item->column() == (int)m_progress_column)
	return;

    QString value = item->text();

    QList<QTableWidgetItem*> twil = m_table->selectedItems();

    for (int i=0; i<twil.size(); ++i)
    {
	QTableWidgetItem *twi = twil[i];
	if (twi != item && (twi->flags() & Qt::ItemIsEditable))
	{
	    twi->setText(value);
	    twi->setForeground(ColourFor(value));
	}
    }
}


std::string CDWindow::QStringToUTF8(const QString& s)
{
    QByteArray qcs = s.toUtf8();
    return std::string(qcs.data(), qcs.length());
}

void CDWindow::OnDone()
{
    unsigned int cdtype = m_cdtype->checkedId();
    unsigned int trackoffset = m_trackoffset->value();

    switch (cdtype)
    {
    case COMPILATION:
	m_rip.SetTemplates("Artists/%A/%t", "Compilations/%c");
	break;
    case MIXED:
	m_rip.SetTemplates("Mixed/%c/%02n %t", "");
	break;
    case SINGLE_ARTIST: default:
	m_rip.SetTemplates("Artists/%A/%c/%02n %t", "");
	break;
    }

    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	db::RecordsetPtr tags = m_rip.Tags(i);

	if (cdtype != COMPILATION)
	{
	    tags->SetInteger(mediadb::TRACKNUMBER, i+1+trackoffset);
	    tags->SetString(mediadb::ALBUM,
			    QStringToUTF8(m_albumname->text()));
	}

	for (unsigned int j=0; j<NUM_COLUMNS; ++j)
	{
	    unsigned int field = columns[j].field;
	    if (!field)
		continue;
	    tags->SetString(field, QStringToUTF8(m_table->item(i,j)->text()));
	}
	
	tags->Commit();
    }

    m_rip.Done();
    
    close();
}

void CDWindow::OnProgress(unsigned track, unsigned type,
			  unsigned num, unsigned denom)
{
    // Called on wrong thread
    QApplication::postEvent(this, new ProgressEvent(track, type, num, denom));
}

void CDWindow::customEvent(QEvent *ce)
{
    if ((int)ce->type() == ProgressEvent::EventType())
    {
	ProgressEvent *pe = (ProgressEvent*)ce;
	unsigned percent
	    = (unsigned)(((long long)pe->GetNum()) * 100 / pe->GetDenom());
	if (percent == 100 && pe->GetNum() != pe->GetDenom())
	    percent = 99;

	unsigned int track = pe->GetTrack();
	unsigned int type = pe->GetType();
	bool draw = false;

	Entry *e = &m_entries[track];

	switch (type)
	{
	case import::PROGRESS_RIP:
	    draw = (percent != e->rt_percent);
	    e->rt_percent = percent;
		break;
	case import::PROGRESS_FLAC:
	    draw = (percent != e->et1_percent);
	    e->et1_percent = percent;
	    break;
	case import::PROGRESS_MP3:
	    draw = (percent != e->et2_percent);
	    e->et2_percent = percent;
	    break;
	}
	
	if (draw)
	{
	    std::ostringstream os;
	    os << e->rt_percent << "%/"
	       << e->et1_percent << "%/"
	       << e->et2_percent << "%";
	    m_table->item(track,m_progress_column)->setText(os.str().c_str());
	}
    }
}

void CDWindow::OnCDDB()
{
    QMenu qp;

    std::map<QAction*, unsigned int> actionmap;
    
    for (unsigned int i=0; i<m_cddb->discs.size(); ++i)
    {
	std::string item = m_cddb->discs[i].title + " by "
	    + m_cddb->discs[i].artist;
	QAction *action = qp.addAction(QString::fromUtf8(item.c_str()));
	actionmap[action] = i;
    }
    qp.addSeparator();
    actionmap[qp.addAction("None of the above")] = 1000;

    QAction *chosen = qp.exec();
    if (chosen)
    {
	if (actionmap.find(chosen) != actionmap.end())
	{
	    unsigned int which = actionmap[chosen];
	    SetUpCDDBStrings(which);
	}
    }
}

void CDWindow::SetUpCDDBStrings(unsigned int which)
{
    if (!m_cddb || which >= m_cddb->discs.size())
    {
	// Clear
	for (unsigned int i=0; i<m_ntracks; ++i)
	{
	    for (unsigned int j=0; j<NUM_COLUMNS; ++j)
	    {
		unsigned int field = columns[j].field;
		if (field == mediadb::TITLE
		    || field == mediadb::ARTIST
		    || field == mediadb::GENRE
		    || field == mediadb::YEAR
		    || field == mediadb::COMMENT)
		    m_table->item(i,j)->setText("");
	    }
	}
	m_albumname->setText("");
    }
    else
    {
	// Populate from CDDB
	const import::CDDBFound *f = &m_cddb->discs[which];
	m_albumname->setText(QString::fromUtf8(f->title.c_str()));
	for (unsigned int i=0; i<m_ntracks; ++i)
	{
	    char year[20];
	    sprintf(year, "%u", f->year);

	    const import::CDDBTrack *t = &f->tracks[i];

	    for (unsigned int j=0; j<NUM_COLUMNS; ++j)
	    {
		unsigned int field = columns[j].field;
		QTableWidgetItem *item = m_table->item(i,j);
		switch (field)
		{
		case mediadb::GENRE:
		    item->setText(QString::fromUtf8(f->genre.c_str()));
		    break;
		case mediadb::YEAR:
		    item->setText(year);
		    break;
		case mediadb::TITLE:
		    item->setText(QString::fromUtf8(t->title.c_str()));
		    break;
		case mediadb::ARTIST:
		    item->setText(QString::fromUtf8(t->artist.empty() ? f->artist.c_str() : t->artist.c_str()));
		    break;
		case mediadb::COMMENT:
		    item->setText(QString::fromUtf8(t->comment.empty() ? f->comment.c_str() : t->comment.c_str()));
		    break;
		}
	    }
	}
    }
}

} // namespace choraleqt

#endif // HAVE_CD
