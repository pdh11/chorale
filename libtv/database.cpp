#include "config.h"
#include "database.h"
#include "dvb.h"
#include "dvb_service.h"
#include "mp3_stream.h"
#include "libmediadb/schema.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#include "libutil/counted_pointer.h"

namespace tv {

Database::Database(dvb::Service *service, dvb::Channels *channels)
    : m_service(service),
      m_channels(channels),
      m_database(mediadb::FIELD_COUNT)
{
    m_database.SetFieldInfo(mediadb::ID, 
			    db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    m_database.SetFieldInfo(mediadb::TYPE, 
			    db::steam::FIELD_INT);
}

unsigned int Database::Init()
{
    db::RecordsetPtr rs = m_database.CreateRecordset();

    std::vector<unsigned int> children;

    if (m_channels)
    {
	unsigned int j=0;
	for (dvb::Channels::const_iterator i = m_channels->begin();
	     i != m_channels->end();
	     ++i, ++j)
	{
	    if (i->videopid == 0 && i->audiopid != 0)
	    {
		rs->AddRecord();
		rs->SetInteger(mediadb::ID, CHANNEL_OFFSET + j);
		rs->SetString(mediadb::TITLE, i->name);
		rs->SetString(mediadb::ARTIST, "Freeview");
		rs->SetInteger(mediadb::TYPE, mediadb::RADIO);
		rs->SetInteger(mediadb::SAMPLERATE, 48000);
		rs->SetInteger(mediadb::BITSPERSEC, 48000*32);
		rs->SetInteger(mediadb::DURATIONMS, 0);
		rs->SetInteger(mediadb::SIZEBYTES, 48000*4*3600*3 + 44);
#if HAVE_MPG123
		rs->SetInteger(mediadb::AUDIOCODEC, mediadb::WAV);
#else
		rs->SetInteger(mediadb::AUDIOCODEC, mediadb::MP2);
#endif
		rs->Commit();

		TRACE << "radio: " << i->name << "\n";
		children.push_back(CHANNEL_OFFSET + j);
	    }
	}
    }

    if (!children.empty())
    {
	rs->AddRecord();
	rs->SetInteger(mediadb::ID, mediadb::RADIO_ROOT);
	rs->SetInteger(mediadb::TYPE, mediadb::DIR);
	rs->SetString(mediadb::TITLE, "Radio");
	rs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(children));
	rs->Commit();
	TRACE << "Added radio root\n";
    }
    else
    {
	TRACE << "No channels\n";
    }

    rs->AddRecord();
    rs->SetInteger(mediadb::ID, mediadb::BROWSE_ROOT);
    rs->SetInteger(mediadb::TYPE, mediadb::DIR);
    rs->SetString(mediadb::TITLE, "Freeview");
    if (!children.empty())
    {
	children.clear();
	children.push_back(mediadb::RADIO_ROOT);
    }
    rs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(children));
    return rs->Commit();
}

std::string Database::GetURL(unsigned int)
{
    return "file:";
}

std::auto_ptr<util::Stream> Database::OpenRead(unsigned int id)
{ 
    std::auto_ptr<util::Stream> sp;

    id -= CHANNEL_OFFSET;
    if (id >= m_channels->Count())
	return sp;
    unsigned int rc = m_service->GetStream(id, &sp);
    if (rc || !sp.get())
	return sp;

#if HAVE_MPG123
    return std::auto_ptr<util::Stream>(new MP3Stream(sp));
#else
    return sp;
#endif
}

std::auto_ptr<util::Stream> Database::OpenWrite(unsigned int)
{
    return std::auto_ptr<util::Stream>();
}

db::QueryPtr Database::CreateQuery()
{
    return m_database.CreateQuery();
}

db::RecordsetPtr Database::CreateRecordset()
{
    return m_database.CreateRecordset();
}

} // namespace tv
