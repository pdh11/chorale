/* cd_progress.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "cd_progress.h"
#include "cd_window.h"
#include "events.h"
#include "settings.h"
#include "libutil/trace.h"
#include <qapplication.h>
#include <qmessagebox.h>

namespace choraleqt {

CDProgress::CDProgress(import::CDDrivePtr drive, const Settings *settings,
		       util::TaskQueue *cpu_queue,
		       util::TaskQueue *disk_queue)
    : QProgressDialog("Reading CD", "Cancel", 0, 2),
      m_drive(drive),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue)
{
    setWindowTitle("Reading CD");
    
    connect(this, SIGNAL(canceled()), 
	    this, SLOT(OnCancel()));

    setAttribute(Qt::WA_DeleteOnClose);

    m_cddb.SetUseProxy(settings->GetUseHttpProxy());
    m_cddb.SetProxyHost(settings->GetHttpProxyHost());
    m_cddb.SetProxyPort(settings->GetHttpProxyPort());

    m_task = import::CDTocTask::Create(drive, &m_cddb);
    m_task->SetObserver(this);
    drive->GetTaskQueue()->PushTask(m_task);
}

CDProgress::~CDProgress()
{
    if (m_task)
	m_task->SetObserver(NULL);
    QApplication::removePostedEvents(this);
    m_task = NULL;
//    TRACE << "~CDProgress\n";
}

void CDProgress::OnCancel()
{
    close();
}

void CDProgress::OnProgress(const util::Task *task, 
			    unsigned num, unsigned denom)
{
    // Called on wrong thread
    QApplication::postEvent(this, new ProgressEvent(task, num, denom));
}

void CDProgress::customEvent(QEvent *ce)
{
    if ((int)ce->type() == EVENT_PROGRESS)
    {
	ProgressEvent *pe = (ProgressEvent*)ce;
	const util::Task *task = pe->GetTask();
	if (task == m_task.get())
	{
	    setValue(pe->GetNum());
	    setMaximum(pe->GetDenom());

	    if (pe->GetNum() == pe->GetDenom())
	    {
		// Done
		if (m_task->IsValid())
		{
		    (void)new CDWindow(m_drive, m_task->GetCD(), 
				       m_task->GetCDDB(), m_settings, 
				       m_cpu_queue, m_disk_queue);
		    close(); // does "delete this", no more member usage!
		}
		else
		{
		    close(); // does "delete this", no more member usage!
		    QMessageBox::warning(NULL, "choralecd",
					 "No audio CD found",
					 "Cancel");
		}
	    }
	}
    }
}

}; // namespace choraleqt
