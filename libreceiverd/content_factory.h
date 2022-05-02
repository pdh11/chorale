#ifndef LIBRECEIVERD_CONTENT_FACTORY_H
#define LIBRECEIVERD_CONTENT_FACTORY_H 1

#include "libutil/http_server.h"
#include <random>

namespace mediadb { class Database; }

/** A server for the Rio Receiver protocol.
 */
namespace receiverd {

class ContentFactory: public util::http::ContentFactory
{
    mediadb::Database *m_db;
    std::default_random_engine m_random;

public:
    explicit ContentFactory(mediadb::Database *db);

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request *rq, 
		       util::http::Response *rs) override;
};

} // namespace receiverd

#endif

