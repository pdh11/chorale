#ifndef LIBRECEIVERD_CONTENT_FACTORY_H
#define LIBRECEIVERD_CONTENT_FACTORY_H 1

#include "libutil/http_server.h"

namespace mediadb { class Database; }

/** A server for the Rio Receiver protocol.
 */
namespace receiverd {

class ContentFactory: public util::http::ContentFactory
{
    mediadb::Database *m_db;

public:
    explicit ContentFactory(mediadb::Database *db) : m_db(db) {}

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request *rq, 
		       util::http::Response *rs);
};

} // namespace receiverd

#endif

