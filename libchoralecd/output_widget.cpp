/* output_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "config.h"
#include "output_widget.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qaction.h>
#include <qmenu.h>
#include <qevent.h>
#include "setlist_window.h"
#include "libutil/trace.h"
#include "libutil/bind.h"
#include "libutil/http.h"
#include "libutil/counted_pointer.h"
#include "liboutput/gstreamer.h"
#include "liboutput/upnpav.h"
#include "liboutput/registry.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/construct.hpp>
#include <algorithm>
#include "imagery/tv.xpm"

namespace choraleqt {

OutputWidget::OutputWidget(QWidget *parent, const std::string &host,
			   QPixmap pm, output::Queue *queue,
			   mediadb::Registry *registry, 
			   const std::string& tooltip)
    : ResourceWidget(parent, host, pm, "Set-list", "Player", tooltip),
      m_queue(queue),
      m_registry(registry),
      m_setlist(NULL)
{
    EnableBottom(false);
}

OutputWidget::~OutputWidget()
{
    delete m_setlist;
}

void OutputWidget::OnTopButton()
{
    if (!m_setlist)
	m_setlist = new SetlistWindow(m_queue, m_registry);
    m_setlist->raise();
    m_setlist->show();
}

void OutputWidget::OnBottomButton()
{
}


        /* UpnpOutputWidget */

UpnpOutputWidget::UpnpOutputWidget(QWidget *parent, const std::string& name, 
				   QPixmap pm, output::Queue *queue, 
				   mediadb::Registry *database_registry,
				   const std::string& tooltip)
    : OutputWidget(parent, name, pm, queue, database_registry, tooltip),
      m_show_properties_action(new QAction("&Properties", this))
{
    EnableTop(false);
    connect(m_show_properties_action, SIGNAL(triggered()), 
	    this, SLOT(showProperties()));
}

unsigned int UpnpOutputWidget::OnInitialised(unsigned int rc)
{
    if (!rc)
    {
	output::Queue *queue = GetQueue();
	output::upnpav::URLPlayer *player
	    = (output::upnpav::URLPlayer*)queue->GetPlayer();
	std::string friendly_name = player->GetFriendlyName();
	queue->SetName(friendly_name);
	SetLabel(friendly_name);
	setEnabled(true);
	if (player->SupportsVideo())
	    SetResourcePixmap(QPixmap(tv_xpm));
    }
    EnableTop(rc == 0);
    return 0;
}

void UpnpOutputWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(m_show_properties_action);
    menu.exec(event->globalPos());
}

void UpnpOutputWidget::showProperties()
{
    
}


        /* UpnpOutputWidgetFactory */


UpnpOutputWidgetFactory::UpnpOutputWidgetFactory(QPixmap *pixmap,
						 mediadb::Registry *dbregistry,
						 output::Registry *opregistry,
						 util::Scheduler *poller,
						 util::http::Client *client,
						 util::http::Server *server)
    : m_pixmap(pixmap),
      m_parent(NULL),
      m_db_registry(dbregistry),
      m_output_registry(opregistry),
      m_poller(poller),
      m_client(client),
      m_server(server)
{
}

void UpnpOutputWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void UpnpOutputWidgetFactory::OnService(const std::string& url,
					const std::string& udn)
{
    std::string output_name = "upnpav:" + udn;
    
    output::Queue *queue = m_output_registry->Get(output_name);

    if (!queue)
    {
	queue = new output::Queue(
	    new output::upnpav::URLPlayer(m_client, m_server, m_poller));
	m_output_registry->Add(output_name, queue);
    }

    // Use the hostname as a stand-in until we know the friendly-name
    std::string hostpart, pathpart;
    util::http::ParseURL(url, &hostpart, &pathpart);
    std::string host;
    unsigned short port;
    util::http::ParseHost(hostpart, 80, &host, &port);

    UpnpOutputWidget *widget = m_widgets[udn];
    if (!widget) 
    {
	widget = new UpnpOutputWidget(m_parent, host, *m_pixmap, queue, 
				      m_db_registry, url);
	m_widgets[udn] = widget;

	widget->setEnabled(false);
    }
    else
    {
	if (widget->isEnabled())
	    return;
    }

    output::upnpav::URLPlayer *player
	    = (output::upnpav::URLPlayer*)queue->GetPlayer();

    /* Re-Init()-ing updates the URL, in case the port number has changed
     */
    player->Init(url, udn,
		 util::Bind(widget).To<unsigned int,
		     &UpnpOutputWidget::OnInitialised>());
}

void UpnpOutputWidgetFactory::OnServiceLost(const std::string&, 
					    const std::string& udn)
{
    widgets_t::const_iterator i = m_widgets.find(udn);
    if (i != m_widgets.end())
    {
	i->second->setEnabled(false);
    }
}

} // namespace choraleqt
