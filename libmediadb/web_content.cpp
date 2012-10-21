#include "web_content.h"
#include "registry.h"
#include "db.h"
#include "schema.h"

namespace mediadb {

bool WebContent::StreamForPath(const util::WebRequest *rq, 
			       util::WebResponse *rs)
{
    unsigned int dbid, id;
    if (sscanf(rq->path.c_str(), "/dbcontent/%x/%x", &dbid, &id) == 2)
    {
	mediadb::Database *db = m_registry->DBForIndex(dbid);
	if (db)
	{
	    db::QueryPtr qp = db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
	    db::RecordsetPtr rsp = qp->Execute();
	    if (rsp && !rsp->IsEOF())
	    {
		rs->ssp = db->OpenRead(id);
		switch (rsp->GetInteger(mediadb::CODEC))
		{
		case mediadb::MP2: rs->content_type = "audio/x-mp2"; break;
		case mediadb::MP3: rs->content_type = "audio/mpeg"; break;
		case mediadb::FLAC: rs->content_type = "audio/x-flac"; break;
		case mediadb::OGGVORBIS: rs->content_type = "application/ogg"; break;
		case mediadb::PCM: rs->content_type = "audio/L16"; break;
		}
		return true;
	    }
	}
    }
    return false;
}

} // namespace mediadb
