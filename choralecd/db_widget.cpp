/* db_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "features.h"
#include "db_widget.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include "libmediatree/root.h"
#include "libmediadb/registry.h"
#include "explorer_window.h"
#include "libdbreceiver/db.h"
#include "libdbupnp/db.h"

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


#ifdef HAVE_LIBDBRECEIVER
ReceiverDBWidgetFactory::ReceiverDBWidgetFactory(QPixmap *pixmap,
						 mediadb::Registry *registry)
    : m_pixmap(pixmap),
      m_registry(registry)
{
}

void ReceiverDBWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void ReceiverDBWidgetFactory::OnService(const util::IPEndPoint& ep)
{
    db::receiver::Database *thedb = new db::receiver::Database;
    thedb->Init(ep);

    (void) new choraleqt::DBWidget(m_parent, "Receiver server", *m_pixmap,
				   thedb, m_registry);
}
#endif


        /* UpnpDBWidgetFactory */


#ifdef HAVE_LIBDBUPNP
UpnpDBWidgetFactory::UpnpDBWidgetFactory(QPixmap *pixmap,
					 mediadb::Registry *registry)
    : m_pixmap(pixmap),
      m_registry(registry)
{
}

void UpnpDBWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void UpnpDBWidgetFactory::OnService(const std::string& url,
				    const std::string& udn)
{
    db::upnpav::Database *thedb = new db::upnpav::Database;
    thedb->Init(url, udn);

    (void) new choraleqt::DBWidget(m_parent, thedb->GetFriendlyName(),
				   *m_pixmap, thedb, m_registry);
}
#endif

} // namespace choraleqt
