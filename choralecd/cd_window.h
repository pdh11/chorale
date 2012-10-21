#ifndef CD_WINDOW_H
#define CD_WINDOW_H

#include <qdialog.h>
#include "libimport/audio_cd.h"
#include "libimport/cddb_service.h"
#include "libutil/task.h"

namespace util {
class TaskQueue;
};
class Settings;
class Q3ButtonGroup;
class QSpinBox;
class QLineEdit;

namespace choraleqt {

class TagTable;

class CDWindow: public QDialog, public util::TaskObserver
{
    Q_OBJECT;

    struct Entry;

    const Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;

    unsigned m_ntracks;
    Entry *m_entries;

    /** For "Album 1", "Album 2" etc */
    static unsigned int sm_index;

    enum {
        SINGLE_ARTIST,
        COMPILATION,
        MIXED
    };
    Q3ButtonGroup *m_cdtype;
    QLineEdit *m_albumname;
    QSpinBox *m_trackoffset;
    TagTable *m_table;
    unsigned int m_progress_column;
    import::CDDBLookupPtr m_cddb;

    std::string QStringToUTF8(const QString& s);
    
    void SetUpCDDBStrings(unsigned int which);

public:
    CDWindow(import::CDDrivePtr drive, import::AudioCDPtr cd,
	     import::CDDBLookupPtr cddb, const Settings*,
	     util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue);
    ~CDWindow();

    void customEvent(QEvent*);

    // Being a TaskObserver
    void OnProgress(const util::Task*,unsigned,unsigned);

public slots:
    void OnDone();
    void OnCDDB();
    void OnTableValueChanged(int,int);
};

}; // namespace choraleqt

#endif
