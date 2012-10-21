/* setlist_window.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "setlist_window.h"
#include "tagtable.h"
#include "browse_widget.h"
#include "events.h"
#include "libutil/trace.h"
#include "libmediadb/registry.h"
#include "libmediadb/schema.h"
#include "libmediadb/db.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlcdnumber.h>
#include <qapplication.h>
#include <vector>
#include <boost/format.hpp>

#include "imagery/play.xpm"
#include "imagery/pause.xpm"
#include "imagery/stop.xpm"
#include "imagery/rew.xpm"
#include "imagery/ffwd.xpm"

namespace choraleqt {

#define UTF8_PLAY  "\xE2\x96\xBA" /* U+25BA */
#define UTF8_PAUSE "\xE2\x95\x91" /* U+2551 */
#define UTF8_STOP  "\xE2\x9D\x9A" /* U+275A */

SetlistWindow::SetlistWindow(output::Queue *queue, mediadb::Registry *registry)
    : QDialog(NULL, NULL),
      m_queue(queue),
      m_registry(registry),
      m_timecode_sec(0),
      m_state(output::STOP),
      m_current_index(UINT_MAX-1)
{
    setWindowTitle("Playback on " + QString::fromUtf8(queue->GetName().c_str()));

    QVBoxLayout *vlayout = new QVBoxLayout(this, 0, 6);

    m_toplayout = new QHBoxLayout(NULL, 0, 0);

    QPixmap stop_pixmap((const char**)stop_xpm);
    m_stop_button = new QPushButton(stop_pixmap, "", this);
    m_toplayout->addWidget(m_stop_button);
    connect(m_stop_button, SIGNAL(clicked()),
	    this, SLOT(Stop()));

    QPixmap rew_pixmap((const char**)rew_xpm);
    QPushButton *rew = new QPushButton(rew_pixmap, "", this);
    m_toplayout->addWidget(rew);
    connect(rew, SIGNAL(clicked()), this, SLOT(Rewind()));

    QPixmap pause_pixmap((const char**)pause_xpm);
    m_pause_button = new QPushButton(pause_pixmap, "", this);
    m_toplayout->addWidget(m_pause_button);
    connect(m_pause_button, SIGNAL(clicked()),
	    this, SLOT(Pause()));

    QPixmap play_pixmap((const char**)play_xpm);
    m_play_button = new QPushButton(play_pixmap, "", this);
    m_toplayout->addWidget(m_play_button);
    connect(m_play_button, SIGNAL(clicked()),
	    this, SLOT(Play()));

    QPixmap ffwd_pixmap((const char**)ffwd_xpm);
    QPushButton *ffwd = new QPushButton(ffwd_pixmap, "", this);
    m_toplayout->addWidget(ffwd);
    connect(ffwd, SIGNAL(clicked()), this, SLOT(FastForward()));

    m_timecode = new QLCDNumber(this);
    m_timecode->setSegmentStyle(QLCDNumber::Flat);
    m_timecode->setFrameStyle(0);
//    m_timecode->setSmallDecimalPoint(true);
    m_timecode->setNumDigits(4);
    m_timecode->display("0:00");
    m_toplayout->addWidget(m_timecode);

//    QPushButton *rb1 = new QPushButton("Shuffle", this);
//    rb1->setToggleButton(true);
//    m_toplayout->addWidget(rb1);

    m_toplayout->addStretch(1);

    vlayout->addLayout(m_toplayout);

    m_table = new TagTable(0, 2, this);

    m_table->setReadOnly(true);
    m_table->setColumnStretchable(0, true);
    m_table->setSorting(false);
    m_table->setShowGrid(false);
    m_table->setSelectionMode(Q3Table::MultiRow);
    m_table->setRowMovingEnabled(true);

    vlayout->addWidget(m_table, 1);
    raise();
    show();

    setMaximumHeight(	m_toplayout->sizeHint().height()
		      + m_table->horizontalHeader()->sizeHint().height()
		      + m_table->horizontalScrollBar()->sizeHint().height()
		      + m_table->contentsHeight() + 4 + 2*6 );

    setAcceptDrops(true);

    queue->AddObserver(this);
}

SetlistWindow::~SetlistWindow()
{
}

void SetlistWindow::OnTimeCode(unsigned int index, unsigned int sec)
{
    // Called on wrong thread
    QApplication::postEvent(this, new TimeCodeEvent(index, sec));
}

void SetlistWindow::OnPlayState(output::PlayState state)
{
    // Called on wrong thread
    QApplication::postEvent(this, new PlayStateEvent(state));
}

void SetlistWindow::customEvent(QEvent *e)
{
    if ((int)e->type() == EVENT_TIMECODE)
    {
	TimeCodeEvent *tce = (TimeCodeEvent*)e;
	unsigned int index = tce->GetIndex();
	unsigned int sec = tce->GetSeconds();

	if (index != m_current_index)
	{
	    BoldableTableItem *bti;
	    if (m_current_index < (unsigned)m_table->numRows())
	    {
		bti = (BoldableTableItem*)m_table->item(m_current_index, 0);
		if (bti)
		    bti->SetBold(false);
	    }
	    if (index < (unsigned)m_table->numRows())
	    {
		bti = (BoldableTableItem*)m_table->item(index, 0);
		if (bti)
		    bti->SetBold(true);
	    }
	    m_table->updateCell(index, 0);
	    m_table->updateCell(m_current_index, 0);
	}

	m_current_index = index;
	m_timecode_sec = sec;
	UpdateCaption();

	std::string s = (boost::format("%u:%02u") % (sec/60) % (sec%60)).str();
//	TRACE << "Updating timecode: " << s << "\n";
	m_timecode->display(s.c_str());
    }
    else if ((int)e->type() == EVENT_PLAYSTATE)
    {
	PlayStateEvent *pse = (PlayStateEvent*)e;
	output::PlayState state = pse->GetState();
	if (state != m_state)
	{
	    m_play_button->setDown(state == output::PLAY);
	    m_pause_button->setDown(state == output::PAUSE);
	    m_stop_button->setDown(state == output::STOP);
	    m_state = state;
	}
	UpdateCaption();
    }
}

void SetlistWindow::UpdateCaption()
{
    unsigned int sec = m_timecode_sec;
    std::string s = (boost::format("%u:%02u") % (sec/60) % (sec%60)).str();
    m_timecode->display(s.c_str());

    s += " ";
    switch (m_state)
    {
    case output::PAUSE: s += UTF8_PAUSE; break;
    case output::PLAY:  s += UTF8_PLAY; break;
    case output::STOP:  s += UTF8_STOP; break;
    }
    s += " ";
    s += m_table->text(m_current_index, 0).utf8().data();
    s += " on " + m_queue->GetName();
    setCaption(QString::fromUtf8(s.c_str()));
}

void SetlistWindow::dragEnterEvent(QDragEnterEvent *e)
{
    bool want = e->mimeData()->hasFormat("application/x-chorale-ids");
    TRACE << "want=" << want << "\n";
    e->accept(want);
}

void SetlistWindow::dropEvent(QDropEvent *e)
{
    QByteArray qba = e->mimeData()->data("application/x-chorale-ids");
    std::vector<IDPair> idps;
    size_t count = qba.size() / sizeof(IDPair);
    idps.resize(count);
    memcpy(&idps[0], qba.data(), count*sizeof(IDPair));
    bool ok = false;

    for (size_t i=0; i<count; ++i)
    {
	const IDPair& idp = idps[i];
	mediadb::Database *db = m_registry->DBForIndex(idp.dbid);
	if (db)
	{
	    m_queue->Add(db, idp.fid);
	    ok = true;

	    int new_queue_len = (int)(m_queue->end() - m_queue->begin());
	    m_table->setNumRows(new_queue_len);
	    db::QueryPtr qp = db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, idp.fid));
	    db::RecordsetPtr rs = qp->Execute();
	    m_table->setItem(new_queue_len-1, 0,
			     new BoldableTableItem(m_table,
			     QString::fromUtf8((rs->GetString(mediadb::TITLE)
						+ " ("
						+ rs->GetString(mediadb::ARTIST)
						+ ")").c_str())));
	    unsigned int duration_sec = rs->GetInteger(mediadb::DURATIONMS);
	    duration_sec += 999;
	    duration_sec /= 1000;
	    std::string duration =
		(boost::format("%u:%02u") % (duration_sec/60)
		                          % (duration_sec%60)).str();
	    m_table->setItem(new_queue_len-1, 1,
			     new HiddenNumericTableItem(m_table,
							Q3TableItem::Never,
							duration.c_str(),
							duration_sec));
	}
    }

    setMaximumHeight( m_toplayout->sizeHint().height()
		      + m_table->horizontalHeader()->sizeHint().height()
		      + m_table->horizontalScrollBar()->sizeHint().height()
		      + m_table->contentsHeight() + 4 + 2*6 );
}

void SetlistWindow::Play()
{
    if (m_table->selection(0).isActive())
	m_queue->Seek(m_table->selection(0).topRow(), 0);
    else if (m_current_index >= (unsigned)m_table->numRows())
	m_queue->Seek(0,0);

    TRACE << "Calling Queue::Play\n";
    m_queue->SetPlayState(output::PLAY);
}

void SetlistWindow::Pause()
{
    TRACE << "Calling Queue::Pause\n";
    m_queue->SetPlayState(output::PAUSE);
}

void SetlistWindow::Stop()
{
    TRACE << "Calling Queue::Stop\n";
    m_queue->SetPlayState(output::STOP);
}

void SetlistWindow::Rewind()
{
    if (m_timecode_sec > 10)
	m_queue->Seek(m_current_index, 0);
    else if (m_current_index > 0)
	m_queue->Seek(m_current_index-1, 0);
}

void SetlistWindow::FastForward()
{
    if (m_current_index+1 <= (unsigned)m_table->numRows())
	m_queue->Seek(m_current_index+1, 0);
}

} // namespace choraleqt
