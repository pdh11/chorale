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
#include "libimport/eject_task.h"
#include "libimport/cd_drives.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include "cd_progress.h"

namespace choraleqt {

CDWidget::CDWidget(QWidget *parent, const std::string &device, QPixmap pm,
		   import::CDDrives *drives, Settings *settings,
		   util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue)
    : ResourceWidget(parent, device, pm, "Rip", "Eject"),
      m_device(device),
      m_drives(drives),
      m_settings(settings),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue)
{
}

void CDWidget::OnTopButton() /* "Rip" */
{
    (void) new CDProgress(m_drives->GetDriveForDevice(m_device),
			  m_settings, m_cpu_queue, m_disk_queue);
}

void CDWidget::OnBottomButton() /* "Eject" */
{
    import::CDDrivePtr drive = m_drives->GetDriveForDevice(m_device);
    drive->GetTaskQueue()->PushTask(import::EjectTask::Create(drive));
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
	(void) new CDWidget(parent, i->first, *m_pixmap, m_drives,
			    m_settings, m_cpu_queue, m_disk_queue);
    }
}

} // namespace choraleqt
