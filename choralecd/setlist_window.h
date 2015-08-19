#ifndef SETLIST_WINDOW_H
#define SETLIST_WINDOW_H

#include <QDialog>
#include "liboutput/queue.h"
#include "setlist_model.h"
#include <QTableView>

class QLCDNumber;
class QPushButton;
class QHBoxLayout;

namespace mediadb { class Registry; }

namespace choraleqt {

/** A top-level window representing the set-list, or running-order, for an
 * audio output.
 */
class SetlistWindow: public QDialog, public output::QueueObserver
{
    Q_OBJECT

    output::Queue *m_queue;
    QLCDNumber *m_timecode;
    QHBoxLayout *m_toplayout;

    SetlistModel m_model;
    QTableView m_table_view;

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

    void customEvent(QEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

    // Being a output::QueueObserver
    void OnTimeCode(unsigned int index, unsigned int sec) override;
    void OnPlayState(output::PlayState state) override;

public slots:
    void Play();
    void Pause();
    void Stop();
    void Rewind();
    void FastForward();
    void SetShuffle(bool);
};

} // namespace choraleqt

#endif
