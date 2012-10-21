#ifndef MEDIATREE_ROOT_H
#define MEDIATREE_ROOT_H 1

#include "libutil/counted_object.h"
#include <vector>
#include "node.h"

namespace db { class Database; };

namespace mediatree {

class Root: public Node
{
    db::Database *m_db;

    explicit Root(db::Database*);

    std::vector<NodePtr> m_children;
    
public:
    virtual ~Root() {}

    static NodePtr Create(db::Database*);

    std::string GetName();
    bool IsCompound() { return true; }
    bool HasCompoundChildren();

    EnumeratorPtr GetChildren();
    db::RecordsetPtr GetInfo();
};

}; // namespace mediatree

#endif
