/* db_widget.h */

#ifndef DB_WIDGET_H
#define DB_WIDGET_H

#include <map>
#include "widget_factory.h"
#include "resource_widget.h"
#include "libreceiver/ssdp.h"
#include "libupnp/ssdp.h"
#include "libempeg/discovery.h"

namespace mediadb { class Database; }
namespace mediadb { class Registry; }
namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }

namespace db { namespace upnp { class Database; } }

namespace choraleqt {

/** A widget for the MainWindow's device list, representing a (remote)
 * music server.
 */
class DBWidget: public ResourceWidget
{
    Q_OBJECT
    
    mediadb::Database *m_db;
    mediadb::Registry *m_registry;

public:
    DBWidget(QWidget *parent, const std::string& name, QPixmap,
	     mediadb::Database*, mediadb::Registry *registry);

    void Enable(bool enabled);

    // Being a ResourceWidget
    void OnTopButton();
    void OnBottomButton();
};

/** A WidgetFactory which creates DBWidget items for Rio Receiver servers.
 */
class ReceiverDBWidgetFactory: public WidgetFactory,
			       public receiver::ssdp::Client::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_registry;
    util::http::Client *m_http;

public:
    ReceiverDBWidgetFactory(QPixmap*, mediadb::Registry *m_registry,
			    util::http::Client *http);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a receiver::ssdp::Client::Callback
    void OnService(const util::IPEndPoint&);
};

/** A widget for the MainWindow's device list, representing a (remote)
 * music server.
 */
class UpnpDBWidget: public DBWidget
{
    Q_OBJECT

    db::upnp::Database *m_upnpdb;
    
public:
    UpnpDBWidget(QWidget *parent, const std::string& name, QPixmap,
		 db::upnp::Database*, mediadb::Registry *registry);

    unsigned OnInitialised(unsigned rc);
};

/** A WidgetFactory which creates DBWidget items for UPnP A/V media servers.
 */
class UpnpDBWidgetFactory: public WidgetFactory,
			   public upnp::ssdp::Responder::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_registry;
    util::http::Client *m_client;
    util::http::Server *m_server;
    util::Scheduler *m_poller;
    typedef std::map<std::string, UpnpDBWidget*> widgets_t;
    widgets_t m_widgets;

public:
    UpnpDBWidgetFactory(QPixmap*, mediadb::Registry *m_registry,
			util::http::Client*, util::http::Server*,
			util::Scheduler *poller);
    
    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a upnp::ssdp::Responder::Callback
    void OnService(const std::string& url, const std::string& udn);
    void OnServiceLost(const std::string& url, const std::string& udn);
};

/** A WidgetFactory which creates DBWidget items for Empeg car-players.
 */
class EmpegDBWidgetFactory: public WidgetFactory,
			    public empeg::Discovery::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_registry;
    util::http::Server *m_server;

public:
    EmpegDBWidgetFactory(QPixmap*, mediadb::Registry *m_registry, 
			 util::http::Server*);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a Discovery::Callback
    void OnDiscoveredEmpeg(const util::IPAddress&, const std::string&);
};

} // namespace choraleqt

#endif
