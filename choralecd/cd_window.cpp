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
#include "libutil/trace.h"
#include "libutil/file.h"
#include <qradiobutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <q3buttongroup.h>
#include <q3popupmenu.h>
#include <qlineedit.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qcursor.h>
#include <qstring.h>
#include "tagtable.h"
#include "libimport/ripping_task.h"
#include "libimport/encoding_task_flac.h"
#include "libimport/encoding_task_mp3.h"
#include "libimport/eject_task.h"
#include "libimport/playlist.h"
#include "events.h"
#include "settings.h"
#include <sstream>
#include <iomanip>
#include "libdb/free_rs.h"
#include "libmediadb/schema.h"
#include <fcntl.h>

namespace choraleqt {

unsigned CDWindow::sm_index = 0;

struct CDWindow::Entry
{
    import::RippingTaskPtr rtp;
    unsigned rt_percent;
    import::EncodingTaskPtr etp1;
    unsigned et1_percent;
    import::EncodingTaskPtr etp2;
    unsigned et2_percent;
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
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue),
      m_cddb(cddb)
{
    unsigned int this_index = ++sm_index;

    unsigned total_sectors = cd->GetTotalSectors();

    std::ostringstream caption;
    caption << "CD " << this_index << ": " << drive->GetName()
	    << " (" << (total_sectors / (75*60)) << ":"
	    << std::setw(2) << std::setfill('0')
	    << ((total_sectors/75) % 60) << ")";

    setWindowTitle(caption.str().c_str());

    QVBoxLayout *vlayout = new QVBoxLayout(this);//, 0, 6);

    QHBoxLayout *toplayout = new QHBoxLayout();//NULL, 0, 6);

    QRadioButton *rb1 = new QRadioButton("&Single-artist", this);
    toplayout->addWidget(rb1);

    QRadioButton *rb2 = new QRadioButton("C&ompilation", this);
    toplayout->addWidget(rb2);

    QRadioButton *rb3 = new QRadioButton("&Mixed", this);
    toplayout->addWidget(rb3);

    m_cdtype = new Q3ButtonGroup(this);
    m_cdtype->insert(rb1, SINGLE_ARTIST);
    m_cdtype->insert(rb2, COMPILATION);
    m_cdtype->insert(rb3, MIXED);
    m_cdtype->setButton(SINGLE_ARTIST);

    toplayout->addStretch(10);

    toplayout->addWidget(new QLabel("Album", this));
    m_albumname = new QLineEdit("Album 1", this);
    toplayout->addWidget(m_albumname, 10);

    toplayout->addStretch(10);

    toplayout->addWidget(new QLabel("Track offset", this));

    m_trackoffset = new QSpinBox(0,99,1,this);
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

    m_ntracks = (unsigned int)cd->GetTrackCount();
    m_entries = new Entry[m_ntracks];

    m_table = new TagTable(m_ntracks, (sizeof(columns)/sizeof(*columns)),
			   this);
    vlayout->addWidget(m_table, 1);

    int duration_column = 0;

    for (unsigned int i=0; i<(sizeof(columns)/sizeof(*columns)); ++i)
    {
	m_table->horizontalHeader()->setLabel(i, columns[i].title);
	if (columns[i].field == 0 || columns[i].field == mediadb::DURATIONMS)
	    m_table->setColumnReadOnly(i, true);
	if (columns[i].field == 0)
	    m_progress_column = i;
	if (columns[i].field == mediadb::DURATIONMS)
	    duration_column = i;
    }

    m_table->setSorting(false);

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
			 new HiddenNumericTableItem(m_table, Q3TableItem::Never,
						    os.str().c_str(),
						    sectors));
    }

    if (cddb)
	SetUpCDDBStrings(0);

    connect(m_table, SIGNAL(valueChanged(int,int)),
	    this, SLOT(OnTableValueChanged(int,int)));

    QHBoxLayout *blayout = new QHBoxLayout(NULL, 0, 6);
    blayout->addStretch(1);

//    QPushButton *cancel = new QPushButton("&Cancel", this);
//    cancel->setAutoDefault(false);
//    blayout->addWidget(cancel);
    QPushButton *done = new QPushButton("&Done", this);
    done->setAutoDefault(false);
    blayout->addWidget(done);
    blayout->addSpacing(20); // clear of the resize handle

    vlayout->addLayout(blayout);

    setSizeGripEnabled(true);

    connect(done, SIGNAL(clicked()), this, SLOT(OnDone()));

    show();
    m_cdtype->hide();

    setMaximumHeight( blayout->sizeHint().height()
		      + toplayout->sizeHint().height()
		      + m_table->horizontalHeader()->sizeHint().height()
		      + m_table->horizontalScrollBar()->sizeHint().height()
		      + m_table->contentsHeight() + 4 + 2*6 + 8 );

    size_t newheight = maximumHeight();
    size_t screenheight = QApplication::desktop()->height();

    if (newheight > (screenheight*7/8))
	newheight = screenheight *7/8;
    resize((int)newheight, width());
     
    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	char leaf[32];
	sprintf(leaf, "cd%03ut%02u", this_index, i+1);
	std::string filename = m_settings->GetFlacRoot() + "/" + leaf;

	import::EncodingTaskPtr etp1
	    = import::EncodingTaskFlac::Create(filename + ".flac");
	m_entries[i].etp1 = etp1;
	m_entries[i].et1_percent = 0;
	etp1->SetObserver(this);

	filename = m_settings->GetMP3Root() + "/" + leaf;

	import::EncodingTaskPtr etp2
	    = import::EncodingTaskMP3::Create(filename + ".mp3");
	m_entries[i].etp2 = etp2;
	m_entries[i].et2_percent = 0;
	etp2->SetObserver(this);

	import::RippingTaskPtr rtp = 
	    import::RippingTask::Create(cd, i, filename, etp1, etp2,
					m_cpu_queue,
					m_disk_queue);
	m_entries[i].rtp = rtp;
	m_entries[i].rt_percent = 0;
	rtp->SetObserver(this);
	drive->GetTaskQueue()->PushTask(rtp);
	TRACE << "Pushed task for track " << i << "/" << m_ntracks << "\n";
    }
    drive->GetTaskQueue()->PushTask(import::EjectTask::Create(drive));
    TRACE << "Pushed eject task\n";
}

CDWindow::~CDWindow()
{
    TRACE << "~CDWindow\n";
    for (unsigned int i = 0; i < m_ntracks; ++i)
    {
	m_entries[i].etp1->SetObserver(NULL);
	m_entries[i].etp2->SetObserver(NULL);
	if (m_entries[i].rtp)
	    m_entries[i].rtp->SetObserver(NULL);
    }
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
	QChar ch = s.at(pos);

	if (ch == ' ')
	{
	    if (pos+1 == (unsigned)s.size() || s.at(pos+1) == '.')
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

void CDWindow::OnTableValueChanged( int row, int col )
{
    QString value = m_table->text( row, col );

//    TRACE << "OTVC " << row << "," << col << "\n";

    for ( int i=0; ; ++i )
    {
	Q3TableSelection qts = m_table->selection(i);

	if ( !qts.isActive() )
	{
	    if (!i) // no selection
		m_table->setItem(row, col,
				 new ColouredItem(m_table, value,
						  ColourFor(value)));
	    break;
	}

	for ( int col = qts.leftCol(); col <= qts.rightCol(); ++col )
	    for ( int row = qts.topRow(); row <= qts.bottomRow(); ++row )
	    {
//		TRACE << "considering " << row << "," << col << "\n";
		unsigned int field = columns[col].field;
		if (field != mediadb::DURATIONMS && field != 0) // readonly
		{
		    m_table->setItem(row, col,
				     new ColouredItem(m_table, value,
						      ColourFor(value)));
		}
	    }
    }
}


std::string CDWindow::QStringToUTF8(const QString& s)
{
    QByteArray qcs = s.toUtf8();
    return std::string(qcs.data(), qcs.length());
}

static std::string UnThe(const std::string& s)
{
    if (std::string(s,0,4) == "The ")
	return std::string(s,4);
    return s;
}

void CDWindow::OnDone()
{
    unsigned int cdtype = m_cdtype->selectedId();
    unsigned int trackoffset = m_trackoffset->value();
    std::string album;
    std::string albumroot;
    import::PlaylistPtr pp;

    if (cdtype == COMPILATION)
    {
	album = QStringToUTF8(m_albumname->text());
	albumroot = "Compilations/" + album + ".asx";

	pp = import::Playlist::Create(m_settings->GetMP3Root()
						  + "/" + albumroot);
	pp->Load();
    }

    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	db::RecordsetPtr tags = db::FreeRecordset::Create();

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
	    tags->SetString(field, QStringToUTF8(m_table->text(i, j)));
	}

//	TRACE << "Track " << (i+1+trackoffset) << tags << "\n";

	std::string artist = UnThe(util::ProtectLeafname(tags->GetString(mediadb::ARTIST)));
	album = util::ProtectLeafname(tags->GetString(mediadb::ALBUM));
	std::string title = util::ProtectLeafname(tags->GetString(mediadb::TITLE));
	
	std::ostringstream name;
	// Form filename
	switch (cdtype)
	{
	case SINGLE_ARTIST:
	    name << "Artists/" 
		 << artist << "/"
		 << album << "/"
		 << std::setw(2) << std::setfill('0')
		 << (i+1+trackoffset) << " "
		 << title;
	    albumroot = "Artists/" + artist + "/" + album;
	    break;
	case COMPILATION:
	    name << "Artists/" 
		 << artist << "/"
		 << title;
	    break;
	case MIXED:
	    name << "Mixed/" 
		 << album << "/"
		 << std::setw(2) << std::setfill('0')
		 << (i+1+trackoffset) << " "
		 << title;
	    albumroot = "Mixed/" + album;
	    break;
	}

	std::string filename = "/";
	filename += name.str();

	TRACE << "Renaming to " << filename << ".ext\n";

	std::string flacname = m_settings->GetFlacRoot() + filename + ".flac";
	std::string mp3name  = m_settings->GetMP3Root()  + filename + ".mp3";

	/** Because they are disk-bound not CPU-bound, punt tagging operations
	 * to the disk queue.
	 */
	m_entries[i].etp1->RenameAndTag(flacname, tags, m_disk_queue);
	m_entries[i].etp2->RenameAndTag(mp3name, tags, m_disk_queue);

	if (pp)
	    pp->SetEntry(i+trackoffset, mp3name);
    }

    if (pp)
    {
	pp->Save();
    }

    std::string link = m_settings->GetMP3Root() + "/New Albums/" + album;
    util::MkdirParents(link.c_str());
    util::posix::MakeRelativeLink(link, m_settings->GetMP3Root() + "/" + albumroot);

    close();
}

void CDWindow::OnProgress(const util::Task *task, unsigned num, unsigned denom)
{
    // Called on wrong thread
    QApplication::postEvent(this, new ProgressEvent(task, num, denom));
}

void CDWindow::customEvent(QEvent *ce)
{
    if ((int)ce->type() == EVENT_PROGRESS)
    {
	ProgressEvent *pe = (ProgressEvent*)ce;
	const util::Task *task = pe->GetTask();
	unsigned percent
	    = (unsigned)(((long long)pe->GetNum()) * 100 / pe->GetDenom());
	if (percent == 100 && pe->GetNum() != pe->GetDenom())
	    percent = 99;

	for (unsigned int i=0; i<m_ntracks; ++i)
	{
	    bool draw = false;
	    if (m_entries[i].etp1.get() == task)
	    {
		draw = (percent != m_entries[i].et1_percent);
		m_entries[i].et1_percent = percent;
	    }
	    else if (m_entries[i].etp2.get() == task)
	    {
		draw = (percent != m_entries[i].et2_percent);
		m_entries[i].et2_percent = percent;
	    }
	    else if (m_entries[i].rtp.get() == task)
	    {
		draw = (percent != m_entries[i].rt_percent);
		m_entries[i].rt_percent = percent;
		if (percent == 100)
		{
		    // Abandon references so the object is deleted
		    m_entries[i].rtp->SetObserver(NULL);
		    m_entries[i].rtp = NULL;
		}
	    }
	    if (draw)
	    {
		std::ostringstream os;
		os << m_entries[i].rt_percent << "%/"
		   << m_entries[i].et1_percent << "%/"
		   << m_entries[i].et2_percent << "%";
		m_table->setText(i, m_progress_column, os.str().c_str());
		break;
	    }
	}
    }
}

void CDWindow::OnCDDB()
{
    Q3PopupMenu qp;
    
    for (unsigned int i=0; i<m_cddb->discs.size(); ++i)
    {
	std::string item = m_cddb->discs[i].title + " by "
	    + m_cddb->discs[i].artist;
	qp.insertItem(QString::fromUtf8(item.c_str()), i);
    }
    qp.insertSeparator();
    qp.insertItem("None of the above", 1000);

    int rc = qp.exec(QCursor::pos());
    if (rc >= 0)
	SetUpCDDBStrings(rc);
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
		    m_table->setText(i, j, "");
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
		switch (field)
		{
		case mediadb::GENRE:
		    m_table->setText(i, j, QString::fromUtf8(f->genre.c_str()));
		    break;
		case mediadb::YEAR:
		    m_table->setText(i, j, year);
		    break;
		case mediadb::TITLE:
		    m_table->setText(i, j, QString::fromUtf8(t->title.c_str()));
		    break;
		case mediadb::ARTIST:
		    m_table->setText(i, j, QString::fromUtf8(t->artist.empty() ? f->artist.c_str() : t->artist.c_str()));
		    break;
		case mediadb::COMMENT:
		    m_table->setText(i, j, QString::fromUtf8(t->comment.empty() ? f->comment.c_str() : t->comment.c_str()));
		    break;
		}
	    }
	}
    }
}

} // namespace choraleqt
