#ifndef SETLIST_WINDOW_H
#define SETLIST_WINDOW_H

#include <QDialog>
#include "liboutput/queue.h"

class QLCDNumber;
class QPushButton;
class QHBoxLayout;

namespace mediadb { class Registry; }

namespace choraleqt {

class TagTable;

/** A top-level window representing the set-list, or running-order, for an
 * audio output.
 */
class SetlistWindow: public QDialog, public output::QueueObserver
{
    Q_OBJECT

    output::Queue *m_queue;
    mediadb::Registry *m_registry;
    QLCDNumber *m_timecode;
    QHBoxLayout *m_toplayout;
    TagTable *m_table;
    QPushButton *m_play_button;
    QPushButton *m_pause_button;
    QPushButton *m_stop_button;

    unsigned int m_timecode_sec;
    output::PlayState m_state;
    unsigned int m_current_index;

    void UpdateCaption();

public:
    SetlistWindow(output::Queue*, mediadb::Registry*);
    ~SetlistWindow();

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
    void customEvent(QEvent*);

    // Being a output::QueueObserver
    void OnTimeCode(unsigned int index, unsigned int sec);
    void OnPlayState(output::PlayState state);

public slots:
    void Play();
    void Pause();
    void Stop();
    void Rewind();
    void FastForward();
};

} // namespace choraleqt

#endif
