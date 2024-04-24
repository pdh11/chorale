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
#include <QVBoxLayout>
#include <sstream>
#include "settings_window.h"
#include "widget_factory.h"
#include "libutil/task.h"

namespace choraleqt {

VBoxWidget::VBoxWidget(QWidget *parent)
    : QWidget(parent),
      m_layout(this)
{
    m_layout.setSpacing(0);
    m_layout.setContentsMargins(0,0,0,0);
}

void VBoxWidget::childEvent(QChildEvent *ce)
{
    if (ce->child()->isWidgetType())
    {
	QWidget *w = (QWidget*)ce->child();
	if (ce->added())
	    m_layout.addWidget(w);
	else if (ce->removed())
	    m_layout.removeWidget(w);
    }
}

MainWindow::MainWindow(Settings *settings, util::TaskQueue *cpu_queue,
		       util::TaskQueue *disk_queue)
    : QMainWindow(NULL),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue),
      m_view(NULL),
      m_encode_tasks(0)
{
    QMenu *file = menuBar()->addMenu("&App");
    file->addAction("&Settings...", this, SLOT(ShowSettings()));
    file->addAction("&Quit", qApp, SLOT(quit()));

    statusBar()->showMessage("");

    setCentralWidget(&m_view);
    setWindowTitle("choralecd");

    startTimer(1000);
}

void MainWindow::AddWidgetFactory(WidgetFactory *wf)
{
    wf->CreateWidgets(&m_view);
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
	    statusBar()->showMessage(os.str().c_str());
	}
	else
	    statusBar()->showMessage("");
    }
}

void MainWindow::ShowSettings()
{
    SettingsWindow *sw = new SettingsWindow(m_settings);
    sw->show();
}

} // namespace choraleqt
