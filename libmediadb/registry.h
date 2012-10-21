#ifndef MEDIADB_REGISTRY_H
#define MEDIADB_REGISTRY_H

#include <map>

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

public:
    unsigned int IndexForDB(mediadb::Database*);
    mediadb::Database *DBForIndex(unsigned int);
};

} // namespace mediadb

#endif
