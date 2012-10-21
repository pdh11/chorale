/* cd_progress.h
 *
 * This SOURCE file is hereby placed in the public domain; BINARIES
 * built from it using the Qt library under the GPL are derived works
 * of Qt and must be treated accordingly.
 */
#ifndef CD_PROGRESS_H
#define CD_PROGRESS_H

#include <qprogressdialog.h>
#include "libutil/task.h"
#include "libimport/cd_drives.h"
#include "libimport/cd_toc_task.h"

class Settings;

namespace util {
class TaskQueue;
};

namespace choraleqt {

class CDProgress: public QProgressDialog, public util::TaskObserver
{
    Q_OBJECT;

    import::CDDrivePtr m_drive;
    const Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;

    import::CDDBService m_cddb;
    import::CDTocTaskPtr m_task;

    ~CDProgress();

public:
    CDProgress(import::CDDrivePtr drive, const Settings *settings,
	       util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue);

    // Being a QWidget
    void customEvent(QEvent*);

    // Being a TaskObserver
    void OnProgress(const util::Task*,unsigned,unsigned);

public slots:
    void OnCancel();
};

}; // namespace choraleqt

#endif
