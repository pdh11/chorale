/* libdbupnp/db.h */
#ifndef LIBDBUPNP_DB_H
#define LIBDBUPNP_DB_H

#include <string>
#include "libmediadb/db.h"
#include "connection.h"
#include "libutil/attributes.h"

namespace db {

/** A database representing the files on a remote UPnP A/V MediaServer.
 *
 * In other words, this is Chorale's implementation of the client end of the
 * UPnP A/V MediaServer protocol.
 */
namespace upnp {

/** Implementation of db::Database; also owns the connection to the server.
 */
class Database: public mediadb::Database
{
    Connection m_connection;

public:
    Database(util::http::Client*, util::http::Server*, util::Scheduler*);
    ~Database();

    unsigned Init(const std::string& url, const std::string& udn)
	ATTRIBUTE_DEPRECATED;

    typedef util::Callback1<unsigned int> InitCallback;

    unsigned Init(const std::string& url, const std::string& udn,
		  InitCallback callback);

    const std::string& GetFriendlyName() const
    {
	return m_connection.GetFriendlyName();
    }

    bool IsForbidden() const
    {
	return m_connection.IsForbidden();
    }

    // Being a Database
    RecordsetPtr CreateRecordset();
    QueryPtr CreateQuery();

    // Being a mediadb::Database
    unsigned int AllocateID() { return m_connection.AllocateID(); }
    std::string GetURL(unsigned int id);
    std::auto_ptr<util::Stream> OpenRead(unsigned int id);
    std::auto_ptr<util::Stream> OpenWrite(unsigned int id);
};

} // namespace upnp
} // namespace db

#endif
