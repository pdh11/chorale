#include "registry.h"
#include "db.h"
#include "libutil/trace.h"

namespace mediadb {

unsigned int Registry::IndexForDB(mediadb::Database *db)
{
    map_t::const_iterator i = m_map.find(db);
    if (i != m_map.end())
	return i->second;

    unsigned int result;
    if (m_map.empty())
	result = 1;
    else
	result = m_revmap.rbegin()->first + 1;

    m_map[db] = result;
    m_revmap[result] = db;
    return result;
}

mediadb::Database *Registry::DBForIndex(unsigned int index)
{
    revmap_t::const_iterator i = m_revmap.find(index);
    if (i == m_revmap.end())
    {
	TRACE << "Can't find mediadb #" << index << "\n";
	return NULL;
    }
    return i->second;
}

} // namespace mediadb

#ifdef TEST

int main()
{
    mediadb::Database *db1 = (mediadb::Database*)"foo";
    mediadb::Database *db2 = (mediadb::Database*)"bar";

    mediadb::Registry mr;

    unsigned int id1 = mr.IndexForDB(db1);
    assert(id1 == 1);
    assert(mr.IndexForDB(db1) == id1);
    assert(mr.DBForIndex(id1) == db1);

    unsigned int id2 = mr.IndexForDB(db2);
    assert(id2 != id1);
    assert(mr.IndexForDB(db1) == id1);
    assert(mr.DBForIndex(id1) == db1);
    assert(mr.IndexForDB(db2) == id2);
    assert(mr.DBForIndex(id2) == db2);
    return 0;
}

#endif
