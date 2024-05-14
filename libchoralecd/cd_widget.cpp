/* cd_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "cd_widget.h"
#include "features.h"
#include "libimport/eject_task.h"
#include "libimport/cd_drives.h"
#include "libimport/remote_cd_drive.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include "cd_progress.h"

#if HAVE_CD

namespace choraleqt {

CDWidget::CDWidget(QWidget *parent, import::CDDrivePtr cd, QPixmap pm,
		   Settings *settings,
		   util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue)
    : ResourceWidget(parent, cd->GetName(), pm, "Rip", "Eject"),
      m_cd(cd),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue)
{
    cd->AddObserver(this);
    if (cd->SupportsDiscPresent())
        CDWidget::OnDiscPresent(cd->DiscPresent());
}

void CDWidget::OnTopButton() /* "Rip" */
{
    (void) new CDProgress(m_cd, m_settings, m_cpu_queue, m_disk_queue);
}

void CDWidget::OnBottomButton() /* "Eject" */
{
    m_cd->GetTaskQueue()->PushTask(import::EjectTask::Create(m_cd));
}

void CDWidget::OnDiscPresent(bool whether)
{
    EnableTop(whether);
}


        /* CDWidgetFactory */


CDWidgetFactory::CDWidgetFactory(QPixmap *pixmap, import::CDDrives *drives,
				 Settings *settings,
				 util::TaskQueue *cpu_queue,
				 util::TaskQueue *disk_queue)
    : m_pixmap(pixmap),
      m_drives(drives),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue)
{
}

void CDWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_drives->Refresh();
    for (import::CDDrives::const_iterator i = m_drives->begin();
	 i != m_drives->end();
	 ++i)
    {
	(void) new CDWidget(parent, i->second, *m_pixmap,
			    m_settings, m_cpu_queue, m_disk_queue);
    }
}


        /* UpnpCDWidgetFactory */


UpnpCDWidgetFactory::UpnpCDWidgetFactory(QPixmap *pixmap,
					 Settings *settings,
					 util::TaskQueue *cpu_queue,
					 util::TaskQueue *disk_queue,
					 util::http::Client *client,
					 util::http::Server *server,
					 util::Scheduler *poller)
    : m_pixmap(pixmap),
      m_parent(NULL),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue),
      m_client(client),
      m_server(server),
      m_poller(poller)
{
}

void UpnpCDWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void UpnpCDWidgetFactory::OnService(const std::string& url,
				    const std::string& udn)
{
    import::RemoteCDDrive *cd = new import::RemoteCDDrive(m_client, m_server,
							  m_poller);
    unsigned rc = cd->Init(url, udn);
    if (rc != 0)
    {
        delete cd;
	return;
    }
    import::CDDrivePtr cdp(cd);
    (void) new CDWidget(m_parent, cdp, *m_pixmap, m_settings, m_cpu_queue,
			m_disk_queue);
}

} // namespace choraleqt

#endif // HAVE_CD

