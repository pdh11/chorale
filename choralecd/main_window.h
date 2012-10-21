/* main_window.h */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmainwindow.h>
#include <qpixmap.h>
#include <QVBoxLayout>

namespace util { class TaskQueue; }

/** Classes for Qt-based UIs for Chorale applications
 */
namespace choraleqt {

class Settings;

class VBoxWidget: public QWidget
{
    Q_OBJECT

    QVBoxLayout m_layout;

    void childEvent(QChildEvent*);

public:
    VBoxWidget(QWidget *parent);
};

class WidgetFactory;

/** The main choralecd window, which shows a list of resource (device)
 * widgets.
 *
 * Each resource widget has (up to) two buttons which perform
 * resource-specific actions. A CD widget has a "Rip" button; a media
 * database has an "Explore" button; an audio output has an "Open
 * set-list" button.
 */
class MainWindow: public QMainWindow
{
    Q_OBJECT

    Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;

    VBoxWidget m_view;
    size_t m_encode_tasks;

    void timerEvent(QTimerEvent*);

public:
    MainWindow(Settings*, util::TaskQueue *cpu_queue,
	       util::TaskQueue *task_queue);

    void AddWidgetFactory(WidgetFactory*);

public slots:
    void ShowSettings();
};

} // namespace choraleqt

#endif
