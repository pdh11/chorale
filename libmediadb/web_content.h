#ifndef LIBMEDIADB_WEB_CONTENT_H
#define LIBMEDIADB_WEB_CONTENT_H 1

#include "libutil/http_server.h"

namespace mediadb {

class Registry;

/** A ContentFactory that serves out media files over HTTP.
 *
 * This is intended for databases that aren't intrinsically HTTP-based, such
 * as local and Empeg content. Databases whose files are already available over
 * HTTP, such as UPnP or Receiver servers, don't need this. (Which they signal
 * by overriding mediadb::GetHttpURL.)
 */
class WebContent: public util::http::ContentFactory
{
    Registry *m_registry;

public:
    explicit WebContent(Registry *r) : m_registry(r) {}

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request*, util::http::Response*);
};

} // namespace mediadb

#endif
