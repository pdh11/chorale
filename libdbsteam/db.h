/* libdbsteam/db.h
 */
#ifndef STEAMDB_STEAMDB_H
#define STEAMDB_STEAMDB_H

#include "libdb/db.h"
#include <string>
#include <map>
#include <vector>
#include <set>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace db {

/** Database implementation "by steam", ie just using STL containers.
 *
 * This uses lots of memory, and isn't persistent (see
 * mediadb::WriteXML) but is quick and has the right API, and can be
 * replaced by something more space-efficient later.
 */
namespace steam {

enum {
    FIELD_STRING   = 0x0, ///< The default type
    FIELD_INT      = 0x1,
    FIELD_TYPEMASK = 0x7,
    FIELD_INDEXED  = 0x8
};

class Database: public db::Database
{
    friend class Query;
    friend class Recordset;
    friend class SimpleRecordset;
    friend class CollateRecordset;
    friend class IndexedRecordset;
    friend class OrderedRecordset;
    
    /** Could use a rwlock in theory, but the critical sections are very
     * small.
     */
    boost::recursive_mutex m_mutex;

    unsigned int m_nfields;
    unsigned int m_next_recno;

    struct FieldInfo {
	unsigned int flags;
    };

    std::vector<FieldInfo> m_fields;

    struct FieldValue {
	std::string s;
	unsigned int i;
	bool svalid : 1;
	bool ivalid : 1;

	FieldValue() : svalid(0), ivalid(0) {}
    };

    typedef std::vector<FieldValue> record_t;

    typedef std::map<unsigned int, record_t> records_t;

    records_t m_data;

    typedef std::map<std::string, std::set<unsigned int> > stringindex_t;

    std::vector<stringindex_t> m_stringindexes;

    typedef std::map<unsigned int, std::set<unsigned int> > intindex_t;

    std::vector<intindex_t> m_intindexes;

public:
    explicit Database(int nfields);

    void SetFieldInfo(int which, unsigned int flags)
    {
	m_fields[which].flags = flags;
    }

    // Being a db::Database
    db::RecordsetPtr CreateRecordset();
    db::QueryPtr CreateQuery();
};

void Test();

} // namespace steam
} // namespace db

#endif
