#include "registry.h"
#include "libutil/trace.h"
#include "db.h"

namespace mediadb {

Registry::Registry()
{
}

#if 0
Registry::~Registry()
{
    TRACE << "Deleting databases\n";
    for (const_iterator i = begin(); i != end(); ++i)
	delete i->second;
    TRACE << "Done\n";
}

void Registry::Add(const std::string& name, mediadb::Database *db)
{
    TRACE << "db" << db << " is called '" << name << "'\n";
    m_names[name] = db;
    unsigned int index = (unsigned int)m_map.size() + 1;
    m_map[db] = index;
    m_revmap[index] = db;
}

unsigned int Registry::IndexForDB(mediadb::Database *db) const
{
    map_t::const_iterator i = m_map.find(db);
    if (i != m_map.end())
	return i->second;

    TRACE << "Can't find mediadb\n";
    return 0;
}

mediadb::Database *Registry::DBForIndex(unsigned int index) const
{
    revmap_t::const_iterator i = m_revmap.find(index);
    if (i == m_revmap.end())
    {
	TRACE << "Can't find mediadb #" << index << "\n";
	return NULL;
    }
    return i->second;
}

mediadb::Database* Registry::DatabaseForName(const std::string& name) const
{
    names_t::const_iterator i = m_names.find(name);
    if (i == m_names.end())
    {
	return NULL;
    }
    return i->second;
}
#endif

} // namespace mediadb

#ifdef TEST

# include "fake_database.h"

int main()
{
    mediadb::Database *db1 = new mediadb::FakeDatabase;
    mediadb::Database *db2 = new mediadb::FakeDatabase;

    mediadb::Registry mr;

    mr.Add("foo", db1);
    unsigned int id1 = mr.GetIndex(db1);
    assert(id1 == 1);
    assert(mr.GetIndex(db1) == id1);
    assert(mr.Get(id1) == db1);

    mr.Add("bar", db2);
    unsigned int id2 = mr.GetIndex(db2);
    assert(id2 != id1);
    assert(mr.GetIndex(db1) == id1);
    assert(mr.Get(id1) == db1);
    assert(mr.GetIndex(db2) == id2);
    assert(mr.Get(id2) == db2);
    return 0;
}

#endif
