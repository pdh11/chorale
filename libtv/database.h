#ifndef LIBTV_MEDIA_DATABASE_H
#define LIBTV_MEDIA_DATABASE_H

#include "libdbsteam/db.h"
#include "libmediadb/db.h"

namespace tv {

namespace dvb { class Service; }
namespace dvb { class Channels; }

/** A mediadb::Database consisting of the available TV and radio channels.
 *
 * Can be served out as-is, or merged with a db::local::Database using
 * a db::merge::Database.
 */
class Database final: public mediadb::Database
{
    dvb::Service *m_service;
    dvb::Channels *m_channels;
    db::steam::Database m_database;

    enum { CHANNEL_OFFSET = 0x200 };

public:
    Database(dvb::Service*, dvb::Channels *channels);
    unsigned int Init();

    // Being a db::Database
    db::RecordsetPtr CreateRecordset() override;
    db::QueryPtr CreateQuery() override;

    // Being a mediadb::Database
    unsigned int AllocateID() override { return 0; }
    std::string GetURL(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenRead(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenWrite(unsigned int id) override;
};

} // namespace tv

#endif
