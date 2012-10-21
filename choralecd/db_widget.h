/* db_widget.h */

#ifndef DB_WIDGET_H
#define DB_WIDGET_H

#include <qwidget.h>
#include <qframe.h>
#include "widget_factory.h"
#include "resource_widget.h"
#include "libreceiver/ssdp.h"
#include "libutil/ssdp.h"

namespace mediadb { class Database; }
namespace mediadb { class Registry; }

namespace choraleqt {

class DBWidget: public ResourceWidget
{
    Q_OBJECT;
    
    mediadb::Database *m_db;
    mediadb::Registry *m_registry;

public:
    DBWidget(QWidget *parent, const std::string& name, QPixmap,
	     mediadb::Database*, mediadb::Registry *registry);

    // Being a ResourceWidget
    void OnTopButton();
    void OnBottomButton();
};

class ReceiverDBWidgetFactory: public WidgetFactory,
			       public receiver::ssdp::Client::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_registry;

public:
    ReceiverDBWidgetFactory(QPixmap*, mediadb::Registry *m_registry);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a receiver::ssdp::Client::Callback
    void OnService(const util::IPEndPoint&);
};

class UpnpDBWidgetFactory: public WidgetFactory,
			   public util::ssdp::Client::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_registry;

public:
    UpnpDBWidgetFactory(QPixmap*, mediadb::Registry *m_registry);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a util::ssdp::Client::Callback
    void OnService(const std::string& url);
};

}; // namespace choraleqt

#endif
