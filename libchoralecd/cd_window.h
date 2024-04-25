#ifndef CD_WINDOW_H
#define CD_WINDOW_H

#include <qdialog.h>
#include "libimport/cddb_service.h"
#include "libimport/ripping_control.h"
#include "libutil/counted_pointer.h"

namespace util { class TaskQueue; }
namespace import { class CDDrive; }
namespace import { class AudioCD; }

class QButtonGroup;
class QSpinBox;
class QLineEdit;
class QTableWidget;
class QTableWidgetItem;

namespace choraleqt {

class Settings;

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
    QButtonGroup *m_cdtype;
    QLineEdit *m_albumname;
    QSpinBox *m_trackoffset;
    QTableWidget *m_table;
    unsigned int m_progress_column;
    import::CDDBLookupPtr m_cddb;

    std::string QStringToUTF8(const QString& s);
    
    void SetUpCDDBStrings(unsigned int which);

public:
    CDWindow(util::CountedPointer<import::CDDrive> drive,
             util::CountedPointer<import::AudioCD> cd,
	     import::CDDBLookupPtr cddb, const Settings*,
	     util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue);
    ~CDWindow();

    void customEvent(QEvent*);

    // Being a RippingControlObserver
    void OnProgress(unsigned,unsigned,unsigned,unsigned);

public slots:
    void OnDone();
    void OnCDDB();
    void OnTableItemChanged(QTableWidgetItem*);
};

} // namespace choraleqt

#endif
