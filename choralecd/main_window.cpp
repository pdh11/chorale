/* main_window.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "main_window.h"
#include <qmenubar.h>
#include <qapplication.h>
#include <qstatusbar.h>
#include <q3vbox.h>
#include <sstream>
#include "settings_window.h"
#include "widget_factory.h"
#include "libutil/task.h"

namespace choraleqt {

MainWindow::MainWindow(Settings *settings, util::TaskQueue *cpu_queue,
		       util::TaskQueue *disk_queue)
    : QMainWindow(NULL, "choralecd"),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue),
      m_view(NULL),
      m_encode_tasks(0)
{
    QMenu *file = new QMenu(this);
    file->insertItem("&Settings...", this, SLOT(ShowSettings()));
    file->insertItem("&Quit", qApp, SLOT(quit()));
    menuBar()->insertItem("&App", file); 

    statusBar()->message("");

    m_view = new Q3VBox(this);

    setCentralWidget(m_view);
    setCaption("choralecd");

    startTimer(1000);
}

void MainWindow::AddWidgetFactory(WidgetFactory *wf)
{
    wf->CreateWidgets(m_view);
}

void MainWindow::timerEvent(QTimerEvent*)
{
    size_t encode_tasks = m_cpu_queue->Count()
	+ m_disk_queue->Count();

    if (encode_tasks != m_encode_tasks)
    {
	m_encode_tasks = encode_tasks;
	if (encode_tasks)
	{
	    std::ostringstream os;
	    os << encode_tasks << " tasks";
	    statusBar()->message(os.str().c_str());
	}
	else
	    statusBar()->message("");
    }
}

void MainWindow::ShowSettings()
{
    SettingsWindow *sw = new SettingsWindow(m_settings);
    sw->show();
}

} // namespace choraleqt
