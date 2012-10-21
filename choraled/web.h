#ifndef CHORALED_WEB_H
#define CHORALED_WEB_H

#include "libutil/web_server.h"

namespace mediadb { class Database; }

class RootContentFactory: public util::ContentFactory
{
    mediadb::Database *m_db;

    util::SeekableStreamPtr HomePageStream();

public:
    RootContentFactory(mediadb::Database *db);

    bool StreamForPath(const util::WebRequest *rq, util::WebResponse *rs);
};

#endif
