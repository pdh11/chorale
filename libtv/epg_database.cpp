#include "epg_database.h"
#include "dvb_service.h"
#include "libdb/delegating_rs.h"
#include "libdb/delegating_query.h"
#include "libutil/xmlescape.h"
#include "libutil/xml.h"
#include "libutil/file_stream.h"
#include "libutil/errors.h"
#include "libutil/trace.h"
#include <boost/static_assert.hpp>
#include <time.h>
#include <stdio.h>

namespace tv {

namespace epg {

static const char *const statemap[] = {
    "none",
    "torecord",
    "cancelled",
    "unrecordable",
    "clashing",
    "recording",
    "recorded",
};

enum { NSTATES = sizeof(statemap)/sizeof(statemap[0]) };

BOOST_STATIC_ASSERT((int)NSTATES == (int)STATE_COUNT);

struct RecordData
{
    unsigned int id;
    std::string title;
    std::string description;
    unsigned int start;
    unsigned int end;
    unsigned int channel;
    std::string state;
};

struct TimerData
{
    std::list<RecordData> records;
};

constexpr const char TIMERDB[] = "timerdb";
constexpr const char RECORD[] = "record";
constexpr const char S_ID[] = "id";
constexpr const char S_TITLE[] = "title";
constexpr const char S_DESCRIPTION[] = "description";
constexpr const char S_START[] = "start";
constexpr const char S_END[] = "end";
constexpr const char S_CHANNEL[] = "channel";
constexpr const char S_STATE[] = "state";

typedef xml::Parser<
    xml::Tag<TIMERDB,
	     xml::List<RECORD, RecordData,
		       TimerData, &TimerData::records,
		       xml::TagMemberInt<S_ID, RecordData,
				      &RecordData::id>,
		       xml::TagMember<S_TITLE, RecordData,
				      &RecordData::title>,
		       xml::TagMember<S_DESCRIPTION, RecordData,
				      &RecordData::description>,
		       xml::TagMemberInt<S_START, RecordData,
					 &RecordData::start>,
		       xml::TagMemberInt<S_END, RecordData,
					 &RecordData::end>,
		       xml::TagMemberInt<S_CHANNEL, RecordData,
					 &RecordData::channel>,
		       xml::TagMember<S_STATE, RecordData,
				      &RecordData::state>
> > > TimerDBParser;
		      


        /* Recordset */


/** A Recordset that overrides Commit to update the on-disk database.
 */
class Recordset final: public db::DelegatingRecordset
{
    Database *m_parent;

public:
    Recordset(db::RecordsetPtr rs, Database *parent)
	: DelegatingRecordset(rs),
	  m_parent(parent)
    {}

    // Being a Recordset
    unsigned int Commit() override;
};

unsigned int Recordset::Commit()
{
    unsigned int id = GetInteger(tv::epg::STATE);
    unsigned int rc = db::DelegatingRecordset::Commit();
    if (rc || id == NONE)
	return rc;

    return m_parent->Rewrite();
}


        /* Query */


/** A Query that wraps all returned Recordsets in a tv::epg::Recordset
 */
class Query final: public db::DelegatingQuery
{
    Database *m_parent;

public:
    Query(db::QueryPtr qp, Database *parent)
	: db::DelegatingQuery(qp),
	  m_parent(parent)
    {
    }

    // Being a Query
    db::RecordsetPtr Execute() override;
};

db::RecordsetPtr Query::Execute()
{
    db::RecordsetPtr rs = db::DelegatingQuery::Execute();
    if (!rs)
	return rs;
    return db::RecordsetPtr(new Recordset(rs, m_parent));
}


       /* Database */


Database::Database(const char *filename, dvb::Service *service)
    : m_filename(filename),
      m_db(tv::epg::FIELD_COUNT)
{
    m_db.SetFieldInfo(tv::epg::ID, 
		      db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(tv::epg::START, 
		      db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(tv::epg::END, 
		      db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(tv::epg::CHANNEL,
		      db::steam::FIELD_INT|db::steam::FIELD_INDEXED);

    std::unique_ptr<util::Stream> ssp;
    unsigned int rc = util::OpenFileStream(m_filename, util::READ,
					   &ssp);

    time_t now = ::time(NULL);
    
    if (!rc)
    {
	TimerDBParser parser;
	TimerData data;
	parser.Parse(ssp.get(), &data);
	for (std::list<RecordData>::const_iterator i = data.records.begin();
	     i != data.records.end();
	     ++i)
	{
	    db::RecordsetPtr rs = m_db.CreateRecordset();
	    rs->AddRecord();
	    rs->SetInteger(ID, i->id);
	    rs->SetString(TITLE, i->title);
	    rs->SetString(DESCRIPTION, i->description);
	    rs->SetInteger(START, i->start);
	    rs->SetInteger(END, i->end);
	    rs->SetInteger(CHANNEL, i->channel);
	    rs->SetInteger(STATE, NONE);
	    for (unsigned int j=0; j<NSTATES; ++j)
	    {
		if (i->state == statemap[j])
		{
		    rs->SetInteger(STATE, j);
		    break;
		}
	    }
	    rs->Commit();
	    if (rs->GetInteger(STATE) == TORECORD && ((time_t)i->end) >= now)
		service->Record(i->channel, i->start, i->end, i->title);
	}
    }
}

Database::~Database()
{
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr(new Recordset(m_db.CreateRecordset(), this));
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(m_db.CreateQuery(), this));
}

unsigned int Database::Rewrite()
{
    std::string new_filename(m_filename);
    new_filename += ".new";
    FILE *f = fopen(new_filename.c_str(), "w+");
    if (!f)
    {
	TRACE << "Can't write timer database: " << errno << "\n";
	return (unsigned)errno;
    }

    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<timerdb>\n");

    db::QueryPtr qp = CreateQuery();
    qp->Where(qp->Restrict(tv::epg::STATE, db::NE, tv::epg::NONE));

    for (db::RecordsetPtr rs = qp->Execute();
	 rs && !rs->IsEOF(); 
	 rs->MoveNext())
    {
	unsigned int state = rs->GetInteger(tv::epg::STATE);

	if (state == NONE || state == CANCELLED)
	    continue;

	fprintf(f, "<record><id>%u</id>"
		"<title>%s</title>"
		"<description>%s</description>"
		"<start>%u</start>"
		"<end>%u</end>"
		"<channel>%u</channel>"
		"<state>%s</state></record>\n",
		rs->GetInteger(ID),
		util::XmlEscape(rs->GetString(TITLE)).c_str(),
		util::XmlEscape(rs->GetString(DESCRIPTION)).c_str(),
		rs->GetInteger(START),
		rs->GetInteger(END),
		rs->GetInteger(CHANNEL),
		statemap[state]);
    }
    fprintf(f, "</timerdb>\n");

    if (ferror(f))
    {
	fclose(f);
	unlink(new_filename.c_str());
	return (unsigned)errno;
    }
    fclose(f);

    int rc = ::rename(new_filename.c_str(), m_filename);
    if (rc)
	return (unsigned)errno;
    return 0;
}

} // namespace epg

} // namespace tv

