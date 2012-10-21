#include "cloud_upnp_databases.h"
#include "cloud_database_widget.h"
#include "cloud_window.h"
#include "libdbupnp/db.h"
#include "libmediadb/registry.h"
#include "libutil/bind.h"
#include "libutil/trace.h"

namespace cloud {

class UpnpDatabase
{
    Window *m_parent;
    db::upnpav::Database *m_db;
    QWidget *m_widget;

public:
    UpnpDatabase(Window*, mediadb::Registry*, db::upnpav::Database*);
    ~UpnpDatabase();

    unsigned int OnSelect();
};

UpnpDatabase::UpnpDatabase(Window *parent, mediadb::Registry *registry,
			   db::upnpav::Database *db)
    : m_parent(parent),
      m_db(db),
      m_widget(new DatabaseWidget(parent, registry, db))
{
}

UpnpDatabase::~UpnpDatabase()
{
    delete m_db;
}

unsigned int UpnpDatabase::OnSelect()
{
    m_parent->SetMainWidget(m_widget);
    return 0;
}

UpnpDatabases::UpnpDatabases(Window *parent, QPixmap *pixmap,
			     upnp::ssdp::Responder *ssdp, 
			     mediadb::Registry *registry, 
			     util::http::Client *client,
			     util::http::Server *server,
			     util::Scheduler *poller)
    : m_parent(parent),
      m_pixmap(pixmap),
      m_registry(registry),
      m_client(client),
      m_server(server),
      m_poller(poller)
{
    ssdp->Search(upnp::s_service_type_content_directory, this);
}

void UpnpDatabases::OnService(const std::string& url,
			      const std::string& udn)
{
    db::upnpav::Database *thedb = new db::upnpav::Database(m_client, m_server,
							   m_poller);
    thedb->Init(url, udn);
    TRACE << "udn=" << udn << "\n";
    m_registry->Add("upnp:" + udn, thedb);
    UpnpDatabase *db = new UpnpDatabase(m_parent, m_registry, thedb);

    MenuEntry me;
    me.pixmap = *m_pixmap;
    me.text = thedb->GetFriendlyName();
    me.onselect = util::Bind(db).To<&UpnpDatabase::OnSelect>();
    TRACE << "Appending\n";
    m_parent->AppendMenuEntry(me);
}

} // namespace cloud
