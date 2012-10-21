#include "db.h"
#include "libmediadb/schema.h"
#include "libreceiver/ssdp.h"
#include "libdb/query.h"
#include "libutil/scheduler.h"
#include "libutil/http_client.h"
#include "libutil/trace.h"
#include "libutil/socket.h"

void DumpDB(db::Database *thedb, unsigned int id, unsigned int depth)
{
    db::QueryPtr qp = thedb->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();

    if (rs->IsEOF())
    {
	TRACE << "Can't get id " << id << "\n";
    }
    else
    {
	std::string padding(depth*2, ' ');
	TRACE << padding << id << ": '" << rs->GetString(mediadb::TITLE) << "'\n";
	if (rs->GetInteger(mediadb::TYPE) == mediadb::PLAYLIST)
	{
	    std::string children = rs->GetString(mediadb::CHILDREN);
	    std::vector<unsigned int> childvec;
	    mediadb::ChildrenToVector(children, &childvec);
	    for (unsigned int i=0; i<childvec.size(); ++i)
	    {
		DumpDB(thedb, childvec[i], depth+1);
	    }
	}
    }
}

class SSDPC: public receiver::ssdp::Client::Callback
{
    util::http::Client *m_http;

public:
    explicit SSDPC(util::http::Client *http) : m_http(http) {}

    void OnService(const util::IPEndPoint& ep)
    {
	TRACE << "Found server on " << ep.ToString() << "\n";

	db::receiver::Database *thedb = new db::receiver::Database(m_http);
	thedb->Init(ep);

	DumpDB(thedb, 0x100, 0);
	return;

	db::QueryPtr qp = thedb->CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, 0x100));
	db::RecordsetPtr rs = qp->Execute();

	if (rs->IsEOF())
	{
	    TRACE << "No root playlist\n";
	}
	else
	{
	    TRACE << "Root playlist is '" << rs->GetString(mediadb::TITLE) << "'\n";
	    std::string children = rs->GetString(mediadb::CHILDREN);
	    std::vector<unsigned int> childvec;
	    mediadb::ChildrenToVector(children, &childvec);
	    TRACE << "Children of the root (" << childvec.size() << "): \n";
	    for (unsigned int i=0; i<childvec.size(); ++i)
	    {
		TRACE << "  " << childvec[i] << "\n";
	    }
	}
    }
};

int main()
{
    util::http::Client http;
    SSDPC callback(&http);
    util::BackgroundScheduler poller;
    receiver::ssdp::Client ssdpc;

    ssdpc.Init(&poller, receiver::ssdp::s_uuid_musicserver, &callback);

    while (1)
    {
	poller.Poll(util::BackgroundScheduler::INFINITE_MS);
    }

    return 0;
}
