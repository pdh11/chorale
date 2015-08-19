#ifndef CHORALED_WEB_H
#define CHORALED_WEB_H

#include "libutil/http_server.h"

namespace mediadb { class Database; }

class RootContentFactory: public util::http::ContentFactory
{
    mediadb::Database *m_db;

    std::unique_ptr<util::Stream> HomePageStream();

public:
    RootContentFactory(mediadb::Database *db);

    bool StreamForPath(const util::http::Request *rq,
		       util::http::Response *rs) override;
};

#endif
