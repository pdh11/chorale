#ifndef MEDIATREE_ROOT_H
#define MEDIATREE_ROOT_H 1

#include <vector>
#include <stdint.h>
#include "node.h"

namespace db { class Database; }

namespace mediatree {

class Root: public Node
{
    db::Database *m_db;

    /** Construct a root item.
     *
     * flags is a bitfield of which query-node children to create.
     */
    Root(db::Database*, uint32_t flags);

    std::vector<NodePtr> m_children;
    
public:
    ~Root();

    static NodePtr Create(db::Database*);

    std::string GetName();
    bool IsCompound() { return true; }
    bool HasCompoundChildren();

    EnumeratorPtr GetChildren();
    util::CountedPointer<db::Recordset> GetInfo();
};

} // namespace mediatree

#endif
