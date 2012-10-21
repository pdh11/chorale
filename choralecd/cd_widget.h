/* cd_widget.h */

#ifndef CD_WIDGET_H
#define CD_WIDGET_H

#include <qframe.h>
#include "resource_widget.h"
#include "widget_factory.h"

class QWidget;
class QPixmap;

namespace import { class CDDrives; };
class Settings;
namespace util { class TaskQueue; };

namespace choraleqt {

class CDWidget: public ResourceWidget
{
    Q_OBJECT;

    std::string m_device;
    import::CDDrives *m_drives;

    Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;
    
public:
    CDWidget(QWidget *parent, const std::string& device, QPixmap,
	     import::CDDrives *drives, Settings *settings,
	     util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue);

    // Being a ResourceWidget
    void OnTopButton();
    void OnBottomButton();
};

class CDWidgetFactory: public WidgetFactory
{
    QPixmap* m_pixmap;
    import::CDDrives *m_drives;
    Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;

public:
    CDWidgetFactory(QPixmap*, import::CDDrives *drives, Settings *settings,
		    util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue);
    
    void CreateWidgets(QWidget *parent);
};

}; // namespace choraleqt

#endif
