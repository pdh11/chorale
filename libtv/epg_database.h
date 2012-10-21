#ifndef LIBTV_EPG_DATABASE_H
#define LIBTV_EPG_DATABASE_H

#include "libdbsteam/db.h"

namespace tv {

namespace dvb { class Service; }

/** Classes for Electronic Program Guides (EPG), ie "what's-on" data.
 */
namespace epg {

enum {
    ID = 0,
    TITLE,
    DESCRIPTION,
    START, ///< Unix time_t
    END,   ///< Unix time_t
    CHANNEL, ///< Index into dvb::Channels
    STATE,

    FIELD_COUNT
};

/** Values of STATE field */
enum {
    NONE,
    TORECORD,
    CANCELLED,
    UNRECORDABLE,
    CLASHING,
    RECORDING,
    RECORDED,

    STATE_COUNT
};

class Database: public db::Database
{
    const char *m_filename;
    db::steam::Database m_db;

public:
    Database(const char *filename, dvb::Service *service);
    ~Database();

    unsigned int Init();

    unsigned int Rewrite();

    // Being a db::Database
    db::RecordsetPtr CreateRecordset();
    db::QueryPtr CreateQuery();
};

} // namespace epg

} // namespace tv

#endif
