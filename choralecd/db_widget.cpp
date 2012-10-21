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
#include "libdbreceiver/database.h"
#include "libdbupnp/database.h"
#include "libdbempeg/db.h"
#include "libmediadb/registry.h"
#include "libutil/http.h"
#include "imagery/portcullis.xpm"

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


        /* UpnpDBWidget */


UpnpDBWidget::UpnpDBWidget(QWidget *parent, const std::string& name,
			   QPixmap icon, db::upnp::Database *db,
			   mediadb::Registry *registry)
    : DBWidget(parent, name, icon, db, registry),
      m_upnpdb(db)
{
}

unsigned int UpnpDBWidget::OnInitialised(unsigned int rc)
{
    if (rc)
	setEnabled(false);
    else
    {
	SetLabel(m_upnpdb->GetFriendlyName());
	if (m_upnpdb->IsForbidden())
	{
	    SetResourcePixmap(QPixmap(portcullis_xpm));
	    setEnabled(false);
	}
	else
	    setEnabled(true);
    }
    return 0;
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

    db::upnp::Database *thedb = (db::upnp::Database*)m_registry->Get(dbname);
    if (thedb == NULL)
    {
	thedb = new db::upnp::Database(m_client, m_server, m_poller);
	m_registry->Add(dbname, thedb);
    }

    // Use the hostname as a stand-in until we know the friendly-name
    std::string hostpart, pathpart;
    util::http::ParseURL(url, &hostpart, &pathpart);
    std::string host;
    unsigned short port;
    util::http::ParseHost(hostpart, 80, &host, &port);

    UpnpDBWidget *widget = m_widgets[udn];
    if (!widget)
    {
	widget = new choraleqt::UpnpDBWidget(m_parent, host, 
					     *m_pixmap, thedb, m_registry);
	m_widgets[udn] = widget;
    }
    widget->setEnabled(false);

    /* Re-Init()-ing updates the URL, in case the port number has changed
     */
    thedb->Init(url, udn,
		util::Bind(widget).To<unsigned int,&UpnpDBWidget::OnInitialised>());
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
