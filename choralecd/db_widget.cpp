/* db_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "config.h"
#include "db_widget.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include "explorer_window.h"
#include "libdbreceiver/db.h"
#include "libdbupnp/db.h"
#include "libdbempeg/db.h"
#include "libmediadb/registry.h"

namespace choraleqt {

DBWidget::DBWidget(QWidget *parent, const std::string& name, QPixmap pm,
		   mediadb::Database *thedb, mediadb::Registry *registry)
    : ResourceWidget(parent, name, pm, "Open", "Close"),
      m_db(thedb),
      m_registry(registry)
{
}

void DBWidget::OnTopButton() /* "Open" */
{
    new ExplorerWindow(m_db, m_registry);
}

void DBWidget::OnBottomButton() /* "Close" */
{
}


        /* ReceiverDBWidgetFactory */


ReceiverDBWidgetFactory::ReceiverDBWidgetFactory(QPixmap *pixmap,
						 mediadb::Registry *registry,
						 util::http::Client *http)
    : m_pixmap(pixmap),
      m_parent(NULL),
      m_registry(registry),
      m_http(http)
{
}

void ReceiverDBWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void ReceiverDBWidgetFactory::OnService(const util::IPEndPoint& ep)
{
    std::string dbname = "receiver:" + ep.ToString();

    db::receiver::Database *thedb = (db::receiver::Database*)m_registry->Get(dbname);
    if (thedb == NULL)
    {
	thedb = new db::receiver::Database(m_http);
	thedb->Init(ep);

	m_registry->Add(dbname, thedb);
    }
    (void) new choraleqt::DBWidget(m_parent, "Receiver server", *m_pixmap,
				   thedb, m_registry);
}


        /* UpnpDBWidgetFactory */


UpnpDBWidgetFactory::UpnpDBWidgetFactory(QPixmap *pixmap,
					 mediadb::Registry *registry,
					 util::http::Client *client,
					 util::http::Server *server,
					 util::Scheduler *poller)
    : m_pixmap(pixmap),
      m_parent(NULL),
      m_registry(registry),
      m_client(client),
      m_server(server),
      m_poller(poller)
{
}

void UpnpDBWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void UpnpDBWidgetFactory::OnService(const std::string& url,
				    const std::string& udn)
{
    std::string dbname = "upnpav:" + udn;

    db::upnpav::Database *thedb = (db::upnpav::Database*)m_registry->Get(dbname);
    if (thedb == NULL)
    {
	thedb = new db::upnpav::Database(m_client, m_server, m_poller);
	thedb->Init(url, udn);

	m_registry->Add(dbname, thedb);
    }
    else
    {
	/* Re-Init()-ing updates the URL, in case the port number has changed
	 */
	thedb->Init(url, udn);
	m_widgets[udn]->setEnabled(true);
    }

    widgets_t::const_iterator i = m_widgets.find(udn);
    if (i == m_widgets.end())
    {
	m_widgets[udn] = new choraleqt::DBWidget(m_parent,
						 thedb->GetFriendlyName(),
						 *m_pixmap, thedb, m_registry);
    }
    else
	i->second->setEnabled(true);
}

void UpnpDBWidgetFactory::OnServiceLost(const std::string&, 
					const std::string& udn)
{
    widgets_t::const_iterator i = m_widgets.find(udn);
    if (i != m_widgets.end())
    {
	i->second->setEnabled(false);
    }
}


        /* EmpegDBWidgetFactory */


EmpegDBWidgetFactory::EmpegDBWidgetFactory(QPixmap *pixmap,
					 mediadb::Registry *registry,
					 util::http::Server *server)
    : m_pixmap(pixmap),
      m_parent(NULL),
      m_registry(registry),
      m_server(server)
{
}

void EmpegDBWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void EmpegDBWidgetFactory::OnDiscoveredEmpeg(const util::IPAddress& ip,
					     const std::string& name)
{
    db::empeg::Database *thedb = new db::empeg::Database(m_server);
    thedb->Init(ip);
    (void) new choraleqt::DBWidget(m_parent, name, *m_pixmap, thedb, 
				   m_registry);
}

} // namespace choraleqt
