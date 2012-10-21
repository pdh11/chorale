#ifndef CLOUD_UPNP_DATABASES_H
#define CLOUD_UPNP_DATABASES_H 1

#include "libupnp/ssdp.h"

class QPixmap;
namespace mediadb { class Registry; }
namespace util { namespace http { class Client; } }
namespace util { namespace http { class Server; } }

namespace cloud {

class Window;

class UpnpDatabases: public upnp::ssdp::Responder::Callback
{
    Window *m_parent;
    QPixmap *m_pixmap;
    mediadb::Registry *m_registry;
    util::http::Client *m_client;
    util::http::Server *m_server;

public:
    UpnpDatabases(Window *parent, QPixmap*,
		  upnp::ssdp::Responder*, mediadb::Registry*, 
		  util::http::Client*, util::http::Server*);

    // Being a upnp::ssdp::Responder::Callback
    void OnService(const std::string& url, const std::string& udn);
};

} // namespace cloud

#endif
