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
#include "events.h"
#include "libutil/trace.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlcdnumber.h>
#include <qapplication.h>
#include <QHeaderView>
#include <boost/format.hpp>
#include <set>

#include "imagery/play.xpm"
#include "imagery/pause.xpm"
#include "imagery/stop.xpm"
#include "imagery/rew.xpm"
#include "imagery/ffwd.xpm"
#include "imagery/shuffle.xpm"
#include "imagery/norepeat.xpm"
#include "imagery/repeatone.xpm"
#include "imagery/repeatall.xpm"

namespace choraleqt {

struct TimeCodePayload
{
    unsigned int index;
    unsigned int seconds;
};

class TimeCodeEvent: public CustomEvent<TimeCodePayload>
{
public:
    TimeCodeEvent(unsigned int index, unsigned int seconds)
    {
	m_data.index = index;
	m_data.seconds = seconds;
    }
    
    unsigned GetIndex() const { return m_data.index; }
    unsigned GetSeconds() const { return m_data.seconds; }
};

typedef CustomEvent<output::PlayState> PlayStateEvent;

#define UTF8_PLAY  "\xE2\x96\xBA" /* U+25BA */
#define UTF8_PAUSE "\xE2\x95\x91" /* U+2551 */
#define UTF8_STOP  "\xE2\x9D\x9A" /* U+275A */

SetlistWindow::SetlistWindow(output::Queue *queue, mediadb::Registry *registry)
    : QDialog(NULL, NULL),
      m_queue(queue),
      m_timecode(NULL),
      m_toplayout(NULL),
      m_model(font(), m_queue, registry),
      m_table_view(this),
      m_play_button(NULL),
      m_pause_button(NULL),
      m_stop_button(NULL),
      m_timecode_sec(0),
      m_state(output::STOP),
      m_current_index(UINT_MAX-1)
{
    setWindowTitle("Playback on " + QString::fromUtf8(queue->GetName().c_str()));

    QVBoxLayout *vlayout = new QVBoxLayout(this);

    m_toplayout = new QHBoxLayout(NULL);

    QPixmap stop_pixmap(stop_xpm);
    m_stop_button = new QPushButton(stop_pixmap, "", this);
    m_stop_button->setAutoDefault(false);
    m_toplayout->addWidget(m_stop_button);
    connect(m_stop_button, SIGNAL(clicked()),
	    this, SLOT(Stop()));

    QPixmap rew_pixmap(rew_xpm);
    QPushButton *rew = new QPushButton(rew_pixmap, "", this);
    rew->setAutoDefault(false);
    m_toplayout->addWidget(rew);
    connect(rew, SIGNAL(clicked()), this, SLOT(Rewind()));

    QPixmap pause_pixmap(pause_xpm);
    m_pause_button = new QPushButton(pause_pixmap, "", this);
    m_pause_button->setAutoDefault(false);
    m_toplayout->addWidget(m_pause_button);
    connect(m_pause_button, SIGNAL(clicked()),
	    this, SLOT(Pause()));

    QPixmap play_pixmap(play_xpm);
    m_play_button = new QPushButton(play_pixmap, "", this);
    m_play_button->setAutoDefault(false);
    m_toplayout->addWidget(m_play_button);
    connect(m_play_button, SIGNAL(clicked()),
	    this, SLOT(Play()));

    QPixmap ffwd_pixmap(ffwd_xpm);
    QPushButton *ffwd = new QPushButton(ffwd_pixmap, "", this);
    ffwd->setAutoDefault(false);
    m_toplayout->addWidget(ffwd);
    connect(ffwd, SIGNAL(clicked()), this, SLOT(FastForward()));

    m_toplayout->addSpacing(6);

    QPixmap shuffle_pixmap(shuffle_xpm);
    QPushButton *shuffle = new QPushButton(shuffle_pixmap, "", this);
    shuffle->setCheckable(true);
    shuffle->setAutoDefault(false);
    m_toplayout->addWidget(shuffle);
    connect(shuffle, SIGNAL(toggled(bool)), this, SLOT(SetShuffle(bool)));

    m_toplayout->addSpacing(6);

    QPixmap norepeat_pixmap(norepeat_xpm);
    QPushButton *norepeat =
	new QPushButton(norepeat_pixmap, "", this);
    norepeat->setAutoDefault(false);
    norepeat->setAutoExclusive(true);
    norepeat->setCheckable(true);
    norepeat->setChecked(true);
    m_toplayout->addWidget(norepeat);

   QPushButton *repeatall =
	new QPushButton(QPixmap(repeatall_xpm), "", this);
    repeatall->setAutoDefault(false);
    repeatall->setAutoExclusive(true);
    repeatall->setCheckable(true);
    m_toplayout->addWidget(repeatall);

    QPushButton *repeatone =
	new QPushButton(QPixmap(repeatone_xpm), "", this);
    repeatone->setAutoDefault(false);
    repeatone->setAutoExclusive(true);
    repeatone->setCheckable(true);
    m_toplayout->addWidget(repeatone);

    m_toplayout->addSpacing(6);

    m_timecode = new QLCDNumber(this);
    m_timecode->setSegmentStyle(QLCDNumber::Flat);
    m_timecode->setFrameStyle(0);
    m_timecode->setDigitCount(4);
    m_timecode->display("-:--");
    m_toplayout->addWidget(m_timecode);

    m_toplayout->addStretch(1);

    vlayout->addLayout(m_toplayout);

    m_table_view.setModel(&m_model);
    m_table_view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table_view.setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table_view.setDragEnabled(true);
    m_table_view.setAcceptDrops(true);
    m_table_view.setDropIndicatorShown(true);
    m_table_view.setShowGrid(false);
    m_table_view.setDragDropOverwriteMode(false);
    m_table_view.setAlternatingRowColors(true);

    m_table_view.horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table_view.horizontalHeader()->hide();
    m_table_view.verticalHeader()->setDefaultSectionSize(
	(QFontInfo(font()).pixelSize() * 5) /4);
    m_table_view.verticalHeader()->hide();

    vlayout->addWidget(&m_table_view, 1);

//    setMaximumHeight(	m_toplayout->sizeHint().height()
//		      + m_table->horizontalHeader()->sizeHint().height()
//		      + m_table->horizontalScrollBar()->sizeHint().height()
//		      + m_table->contentsHeight() + 4 + 2*6 );

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
    if ((int)e->type() == TimeCodeEvent::EventType())
    {
	TimeCodeEvent *tce = (TimeCodeEvent*)e;
	unsigned int index = tce->GetIndex();
	unsigned int sec = tce->GetSeconds();

	m_model.SetIndexAndTimecode(index, sec);

	m_current_index = index;
	m_timecode_sec = sec;
	UpdateCaption();
    }
    else if ((int)e->type() == PlayStateEvent::EventType())
    {
	PlayStateEvent *pse = (PlayStateEvent*)e;
	output::PlayState state = pse->GetData();
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
    std::string s;
    if (sec >= 3600)
	s = (boost::format("%u:%02u:%02u")
	     % (sec/3600)
	     % ((sec/60)%60) 
	     % (sec%60)).str();
    else
	s = (boost::format("%u:%02u") % (sec/60) % (sec%60)).str();

    m_timecode->display(s.c_str());
    m_timecode->setDigitCount((int)s.length());

    s += " ";
    switch (m_state)
    {
    case output::PAUSE: s += UTF8_PAUSE; break;
    case output::PLAY:  s += UTF8_PLAY; break;
    case output::STOP:  s += UTF8_STOP; break;
    }
    s += " ";
    if (m_state != output::STOP && m_current_index < m_queue->Count())
    {
	s += m_model.Name(m_queue->QueueAt(m_current_index)).toUtf8().data();
	s += " on ";
    }
    s += m_queue->GetName();
    setWindowTitle(QString::fromUtf8(s.c_str()));
}

void SetlistWindow::Play()
{
    int nItems = m_model.rowCount(QModelIndex());

    QItemSelectionModel *qism = m_table_view.selectionModel();
    if (qism->hasSelection())
    {
	QModelIndexList qmil = qism->selectedIndexes();
	int topmost_selected = nItems;
	for (int i=0; i<qmil.size(); ++i)
	{
	    if (qmil[i].row() < topmost_selected)
		topmost_selected = qmil[i].row();
	}
	if (topmost_selected < nItems)
	    m_queue->Seek(topmost_selected, 0); /// @bug output order
    }
    else if (m_current_index >= (unsigned)nItems)
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
    if (m_current_index+1 <= (unsigned)m_model.rowCount(QModelIndex()))
	m_queue->Seek(m_current_index+1, 0);
}

void SetlistWindow::SetShuffle(bool whether)
{
    m_queue->SetShuffle(whether);
}

void SetlistWindow::keyPressEvent(QKeyEvent *ke)
{
    if (ke->key() == Qt::Key_Backspace || ke->key() == Qt::Key_Delete)
    {
	QItemSelectionModel *qism = m_table_view.selectionModel();
	if (qism->hasSelection())
	{
	    std::set<unsigned int> selection;

	    QModelIndexList qmil = qism->selectedIndexes();
	    for (int i=0; i<qmil.size(); ++i)
		selection.insert((unsigned)qmil[i].row());

	    // Remove in reverse order so as not to invalidate earlier indexes
	    for (std::set<unsigned int>::const_reverse_iterator i = 
		     selection.rbegin();
		 i != selection.rend();
		 ++i)
		m_model.removeRows((int)*i, 1, QModelIndex());
	}
    }
    else
	QDialog::keyPressEvent(ke);
}

} // namespace choraleqt
