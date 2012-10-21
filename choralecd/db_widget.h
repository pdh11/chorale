/* db_widget.h */

#ifndef DB_WIDGET_H
#define DB_WIDGET_H

#include "widget_factory.h"
#include "resource_widget.h"
#include "libreceiver/ssdp.h"
#include "libupnp/ssdp.h"
#include "libempeg/discovery.h"

namespace mediadb { class Database; }
namespace mediadb { class Registry; }
namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }

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

public:
    UpnpDBWidgetFactory(QPixmap*, mediadb::Registry *m_registry,
			util::http::Client*, util::http::Server*);
    
    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a upnp::ssdp::Responder::Callback
    void OnService(const std::string& url, const std::string& udn);
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
