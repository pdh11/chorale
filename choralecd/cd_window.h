#ifndef CD_WINDOW_H
#define CD_WINDOW_H

#include <qdialog.h>
#include "libimport/audio_cd.h"
#include "libimport/cd_drives.h"
#include "libimport/cddb_service.h"
#include "libimport/ripping_control.h"

namespace util { class TaskQueue; }
class Settings;
class Q3ButtonGroup;
class QSpinBox;
class QLineEdit;

namespace choraleqt {

class TagTable;

/** The top-level ripping and tagging window for an audio CD.
 */
class CDWindow: public QDialog, public import::RippingControlObserver
{
    Q_OBJECT

    struct Entry;

    import::RippingControl m_rip;

    unsigned m_ntracks;
    Entry *m_entries;

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

    // Being a RippingControlObserver
    void OnProgress(unsigned,unsigned,unsigned,unsigned);

public slots:
    void OnDone();
    void OnCDDB();
    void OnTableValueChanged(int,int);
};

} // namespace choraleqt

#endif
