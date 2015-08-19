#ifndef LIBDBRECEIVER_DB_H
#define LIBDBRECEIVER_DB_H 1

#include "libmediadb/db.h"
#include "connection.h"

namespace util { namespace http { class Client; } }

namespace db {

/** A database representing the files on a remote Rio Receiver server.
 *
 * In other words, this is Chorale's implementation of the client end of the
 * Rio Receiver protocol, for which see http://www.dasnet.org/node/69
 */
namespace receiver {

/** Implementation of db::Database; also owns the connection to the server.
 */
class Database final: public mediadb::Database
{
    Connection m_connection;

public:
    explicit Database(util::http::Client *http);
    ~Database();

    unsigned int Init(const util::IPEndPoint&);

    std::string GetFieldName(int mediadbtag);

    // Being a db::Database
    db::RecordsetPtr CreateRecordset() override;
    db::QueryPtr CreateQuery() override;

    // Being a mediadb::Database
    unsigned int AllocateID() override { return 0; } // read-only, no new IDs
    std::string GetURL(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenRead(unsigned int id) override;
    std::unique_ptr<util::Stream> OpenWrite(unsigned int id) override;
};

} // namespace receiver
} // namespace db

#endif
