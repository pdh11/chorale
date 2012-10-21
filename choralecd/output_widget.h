/* output_widget.h */

#ifndef OUTPUT_WIDGET_H
#define OUTPUT_WIDGET_H

#include <qwidget.h>
#include "resource_widget.h"
#include "widget_factory.h"
#include "libupnp/ssdp.h"
#include "libutil/hal.h"

namespace mediadb { class Registry; }
namespace output { class Registry; }
namespace output { class URLPlayer; }
namespace output { class Queue; }
namespace output { namespace upnpav { class URLPlayer; } }
namespace util { class Scheduler; }
namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }

namespace choraleqt {

class SetlistWindow;

/** A widget for the MainWindow's device list, representing an audio output.
 */
class OutputWidget: public ResourceWidget
{
    Q_OBJECT
    
    output::Queue *m_queue;
    mediadb::Registry *m_registry;
    SetlistWindow *m_setlist;

public:
    OutputWidget(QWidget *parent, const std::string& name, QPixmap,
		 output::Queue*, mediadb::Registry*, 
		 const std::string& tooltip);
    ~OutputWidget();

    output::Queue *GetQueue() { return m_queue; }
    
    // Being a ResourceWidget
    void OnTopButton();
    void OnBottomButton();
};

/** A WidgetFactory which creates OutputWidget items for local ALSA
 * outputs.
 */
class OutputWidgetFactory: public WidgetFactory,
			   public util::hal::DeviceObserver
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    util::hal::Context *m_hal;
    mediadb::Registry *m_registry;
    std::list<output::URLPlayer*> m_players;

public:
    OutputWidgetFactory(QPixmap*, util::hal::Context*, mediadb::Registry*);
    ~OutputWidgetFactory();
    
    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a util::hal::DeviceObserver
    void OnDevice(util::hal::DevicePtr dev);
};

class UpnpOutputWidget: public OutputWidget
{
    Q_OBJECT
    
    output::upnpav::URLPlayer *m_player;
    
public:
    UpnpOutputWidget(QWidget *parent, const std::string& name, QPixmap,
		     output::upnpav::URLPlayer*,
		     output::Queue*, mediadb::Registry*, const std::string&);

    unsigned int OnInitialised(unsigned int error_code);
};

/** A WidgetFactory which creates OutputWidget items for UPnP media
 * renderers.
 */
class UpnpOutputWidgetFactory: public WidgetFactory,
			       public upnp::ssdp::Responder::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_db_registry;
    output::Registry *m_output_registry;
    util::Scheduler *m_poller;
    util::http::Client *m_client;
    util::http::Server *m_server;

public:
    UpnpOutputWidgetFactory(QPixmap*, mediadb::Registry*, 
			    output::Registry*,
			    util::Scheduler*,
			    util::http::Client*, util::http::Server*);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a upnp::ssdp::Responder::Callback
    void OnService(const std::string& url, const std::string& udn);
};

} // namespace choraleqt

#endif
