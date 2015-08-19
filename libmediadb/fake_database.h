#ifndef LIBMEDIADB_FAKE_DATABASE_H
#define LIBMEDIADB_FAKE_DATABASE_H 1

#ifdef TEST

# include "db.h"
# include "allocate_id.h"
# include "libdb/query.h"
# include "libdb/recordset.h"
# include "schema.h"
# include "libdbsteam/db.h"
# include "libutil/string_stream.h"
# include "libutil/counted_pointer.h"
# include <boost/format.hpp>

namespace mediadb {

/** A mediadb::Database whose database works, but which has no files.
 *
 * Useful for testing.
 */
class FakeDatabase: public mediadb::Database
{
    db::steam::Database m_db;
    mediadb::AllocateID m_aid;

public:
    FakeDatabase()
	: m_db(mediadb::FIELD_COUNT),
	  m_aid(&m_db)
    {
	m_db.SetFieldInfo(mediadb::ID, 
			  db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
	m_db.SetFieldInfo(mediadb::PATH,
			  db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
	m_db.SetFieldInfo(mediadb::ARTIST,
			  db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
	m_db.SetFieldInfo(mediadb::ALBUM,
			  db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
	m_db.SetFieldInfo(mediadb::GENRE,
			  db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
	m_db.SetFieldInfo(mediadb::TITLE,
			  db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    }

    // Being a Database
    db::RecordsetPtr CreateRecordset() { return m_db.CreateRecordset(); }
    db::QueryPtr CreateQuery() { return m_db.CreateQuery(); }

    // Being a mediadb::Database
    unsigned int AllocateID()
    {
	return m_aid.Allocate();
    }

    std::string GetURL(unsigned int fid)
    {
	return (boost::format("test:%u") % fid).str();
    }

    std::unique_ptr<util::Stream> OpenRead(unsigned int)
    {
	return std::unique_ptr<util::Stream>(new util::StringStream(""));
    }

    std::unique_ptr<util::Stream> OpenWrite(unsigned int)
    {
	return std::unique_ptr<util::Stream>(new util::StringStream(""));
    }
};

} // namespace mediadb

#endif // TEST

#endif
