#ifndef LIBRECEIVERD_CONTENT_FACTORY_H
#define LIBRECEIVERD_CONTENT_FACTORY_H 1

#include "libutil/web_server.h"
#include "libutil/stream.h"

namespace mediadb { class Database; }

/** A server for the Rio Receiver protocol.
 */
namespace receiverd {

class ContentFactory: public util::ContentFactory
{
    mediadb::Database *m_db;

public:
    explicit ContentFactory(mediadb::Database *db) : m_db(db) {}

    // Being a ContentFactory
    bool StreamForPath(const util::WebRequest *rq, util::WebResponse *rs);
};

} // namespace receiverd

#endif

