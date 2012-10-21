/* cd_widget.h */

#ifndef CD_WIDGET_H
#define CD_WIDGET_H

#include <qframe.h>
#include "resource_widget.h"
#include "widget_factory.h"
#include "libupnp/ssdp.h"
#include "libimport/cd_drives.h"

class QWidget;
class QPixmap;

class Settings;
namespace util { class TaskQueue; }
namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }

namespace choraleqt {

/** A widget for the MainWindow's device list, representing a CD drive.
 */
class CDWidget: public ResourceWidget, public import::CDDriveObserver
{
    Q_OBJECT

    import::CDDrivePtr m_cd;

    Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;
    
public:
    CDWidget(QWidget *parent, import::CDDrivePtr, QPixmap,
	     Settings *settings,
	     util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue);

    // Being a ResourceWidget
    void OnTopButton();
    void OnBottomButton();

    // Being a CDDriveObserver
    void OnDiscPresent(bool);
};

/** A WidgetFactory which creates CDWidget items for local CD drives.
 */
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

/** A WidgetFactory which creates CDWidget items for remote (UPnP)
 * CD drives.
 */
class UpnpCDWidgetFactory: public WidgetFactory,
			   public upnp::ssdp::Responder::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    Settings *m_settings;
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;
    util::http::Client *m_client;
    util::http::Server *m_server;

public:
    UpnpCDWidgetFactory(QPixmap*, Settings *settings,
			util::TaskQueue *cpu_queue,
			util::TaskQueue *disk_queue,
			util::http::Client*,
			util::http::Server*);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a upnp::ssdp::Responder::Callback
    void OnService(const std::string& url, const std::string& udn);
};

} // namespace choraleqt

#endif
