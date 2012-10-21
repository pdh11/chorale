#ifndef MEDIADB_REGISTRY_H
#define MEDIADB_REGISTRY_H

#include <map>
#include <string>

namespace mediadb {

class Database;

/** Central registry to index media databases (within a single program
 * instance).
 *
 * Used by drag-and-drop.
 */
class Registry
{
    typedef std::map<mediadb::Database*, unsigned int> map_t;
    map_t m_map;

    typedef std::map<unsigned int, mediadb::Database*> revmap_t;
    revmap_t m_revmap;

    typedef std::map<std::string, mediadb::Database*> names_t;
    names_t m_names;

public:
    unsigned int IndexForDB(mediadb::Database*);
    mediadb::Database *DBForIndex(unsigned int) const;

    void NameDatabase(mediadb::Database*, const std::string& name);
    mediadb::Database *DatabaseForName(const std::string& name) const;

    typedef names_t::const_iterator const_iterator;
    const_iterator begin() const { return m_names.begin(); }
    const_iterator end() const { return m_names.end(); }
};

} // namespace mediadb

#endif
