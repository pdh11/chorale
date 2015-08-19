/* libdbempeg/db.cpp */

#include "db.h"
#include "rs.h"
#include "query.h"
#include "libdbempeg/connection.h"
#include "libutil/trace.h"

namespace db {
namespace empeg {

Database::Database(util::http::Server *server)
    : m_connection(new Connection(server))
{
}

Database::~Database()
{
    delete m_connection;
}

unsigned int Database::Init(const util::IPAddress& ip)
{
    return m_connection->Init(ip);
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr(new Recordset(m_connection->CreateLocalRecordset(),
					  m_connection));
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(m_connection->CreateLocalQuery(),
				  m_connection));
}

std::unique_ptr<util::Stream> Database::OpenRead(unsigned int id)
{
    return m_connection->OpenRead(id);
}

std::unique_ptr<util::Stream> Database::OpenWrite(unsigned int id)
{
    return m_connection->OpenWrite(id);
}

std::string Database::GetURL(unsigned int id)
{
    return m_connection->GetURL(id);
}

unsigned int Database::AllocateID()
{
    return m_connection->AllocateID();
}

} // namespace db::empeg
} // namespace db

#ifdef TEST

# include "libempeg/discovery.h"
# include "libutil/scheduler.h"
# include "libutil/worker_thread_pool.h"

class DECallback: public empeg::Discovery::Callback
{
    util::http::Server *m_server;

public:
    DECallback(util::http::Server *server) : m_server(server) {}

    void OnDiscoveredEmpeg(const util::IPAddress& ip, const std::string& name)
    {
	TRACE << "Found Empeg '" << name << "' on " << ip.ToString() << "\n";

	empeg::ProtocolClient pc;
	std::string type;
	unsigned int rc = pc.Init(ip);
	if (!rc)
	    rc = pc.ReadFidToString(empeg::FID_TYPE, &type);
	if (rc != 0)
	{
	    TRACE << "Can't connect: " << rc <<" \n";
	    return;
	}

	if (!strcmp(type.c_str(), "jupiter"))
	{
	    TRACE << "It's a Jupiter\n";
	    return;
	}
	db::empeg::Database db(m_server);
	rc = db.Init(ip);
	if (rc != 0)
	    TRACE << "Can't open database: " << rc << "\n";
    }
};

int main(int, char **)
{
    util::BackgroundScheduler poller;
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);
    util::http::Server server(&poller, &wtp);

    empeg::Discovery disc;

    DECallback dec(&server);

    disc.Init(&poller, &dec);

    time_t t = time(NULL);

    while ((time(NULL) - t) < 5)
    {
	poller.Poll(1000);
    }

    return 0;
}

#endif
