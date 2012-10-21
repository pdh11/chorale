/* main_window.h */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmainwindow.h>
#include <qpixmap.h>

class Settings;
namespace util {
class TaskQueue;
};
class Q3VBox;

/** Classes for Qt-based UIs for Chorale applications
 */
namespace choraleqt {

class WidgetFactory;

class MainWindow: public QMainWindow
{
    Q_OBJECT;

    Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;

    Q3VBox *m_view;
    unsigned int m_encode_tasks;

    void timerEvent(QTimerEvent*);

public:
    MainWindow(Settings*, util::TaskQueue *cpu_queue,
	       util::TaskQueue *task_queue);

    void AddWidgetFactory(WidgetFactory*);

public slots:
    void ShowSettings();
};

}; // namespace choraleqt

#endif
