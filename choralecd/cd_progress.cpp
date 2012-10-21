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
#include "features.h"
#include "cd_window.h"
#include "events.h"
#include "settings.h"
#include "libutil/trace.h"
#include <qapplication.h>
#include <qmessagebox.h>

#if HAVE_CD

namespace choraleqt {

CDProgress::CDProgress(import::CDDrivePtr drive, const Settings *settings,
		       util::TaskQueue *cpu_queue,
		       util::TaskQueue *disk_queue)
    : QProgressDialog(QString::fromUtf8("Reading CD"), 
		      QString::fromUtf8("Cancel"), 0, 2),
      m_drive(drive),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue)
{
    setWindowTitle(QString::fromUtf8("Reading CD"));
    
    connect(this, SIGNAL(canceled()), 
	    this, SLOT(OnCancel()));

    setAttribute(Qt::WA_DeleteOnClose);

    m_cddb.SetUseProxy(settings->GetUseHttpProxy());
    m_cddb.SetProxyHost(settings->GetHttpProxyHost());
    m_cddb.SetProxyPort(settings->GetHttpProxyPort());

    m_task = import::CDTocTask::Create(drive, &m_cddb);
    m_task->SetObserver(this);
    drive->GetTaskQueue()->PushTask(
	util::Bind(m_task).To<&import::CDTocTask::Run>());
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
    if (task == m_task.get())
    {
	// Called on wrong thread
	QApplication::postEvent(this, new ProgressEvent(0, 0, num, denom));
    }
}

void CDProgress::customEvent(QEvent *ce)
{
    if ((int)ce->type() == ProgressEvent::EventType())
    {
	ProgressEvent *pe = (ProgressEvent*)ce;

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
		QMessageBox::warning(NULL,
				     QString::fromUtf8("choralecd"),
				     QString::fromUtf8("No audio CD found"),
				     QString::fromUtf8("Cancel"));
	    }
	}
    }
}

} // namespace choraleqt

#endif
