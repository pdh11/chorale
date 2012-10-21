/* output_widget.h */

#ifndef OUTPUT_WIDGET_H
#define OUTPUT_WIDGET_H

#include <qwidget.h>
#include "resource_widget.h"
#include "widget_factory.h"
#include "libutil/ssdp.h"

namespace mediadb { class Registry; }
namespace output { class Queue; }
namespace util { class PollerInterface; }

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
    OutputWidget(QWidget *parent, const std::string& host, QPixmap,
		 output::Queue*, mediadb::Registry*);
    ~OutputWidget();
    
    // Being a ResourceWidget
    void OnTopButton();
    void OnBottomButton();
};

/** A WidgetFactory which creates OutputWidget items for local ALSA
 * outputs.
 */
class OutputWidgetFactory: public WidgetFactory
{
    QPixmap *m_pixmap;
    output::Queue *m_queue;
    mediadb::Registry *m_registry;

public:
    OutputWidgetFactory(QPixmap*, output::Queue*, mediadb::Registry*);
    
    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);
};

/** A WidgetFactory which creates OutputWidget items for UPnP media
 * renderers.
 */
class UpnpOutputWidgetFactory: public WidgetFactory,
			       public util::ssdp::Client::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_registry;
    util::PollerInterface *m_poller;

public:
    UpnpOutputWidgetFactory(QPixmap*, mediadb::Registry*, 
			    util::PollerInterface*);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a util::ssdp::Client::Callback
    void OnService(const std::string& url, const std::string& udn);
};

} // namespace choraleqt

#endif
