/* output_widget.h */

#ifndef OUTPUT_WIDGET_H
#define OUTPUT_WIDGET_H

#include <qwidget.h>
#include "resource_widget.h"
#include "widget_factory.h"
#include "libutil/ssdp.h"

namespace mediadb { class Registry; };
namespace output { class Queue; };

namespace choraleqt {

class SetlistWindow;

class OutputWidget: public ResourceWidget
{
    Q_OBJECT;
    
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

class UpnpOutputWidgetFactory: public WidgetFactory,
			       public util::ssdp::Client::Callback
{
    QPixmap *m_pixmap;
    QWidget *m_parent;
    mediadb::Registry *m_registry;

public:
    UpnpOutputWidgetFactory(QPixmap*, mediadb::Registry*);

    // Being a WidgetFactory
    void CreateWidgets(QWidget *parent);

    // Being a util::ssdp::Client::Callback
    void OnService(const std::string& url);
};

}; // namespace choraleqt

#endif
