#ifndef MEDIATREE_QUERY_H
#define MEDIATREE_QUERY_H 1

#include "libutil/counted_object.h"
#include <vector>
#include "node.h"

namespace db { class Database; }

namespace mediatree {

class Query: public Node
{
    db::Database *m_db;
    int m_field;
    db::RecordsetPtr m_info;

    Query(db::Database*, int field, const std::string& name);

    std::vector<NodePtr> m_children;
    
public:
    ~Query();

    static NodePtr Create(db::Database*, int field, const std::string& name);

    std::string GetName();
    bool IsCompound() { return true; }
    bool HasCompoundChildren();

    EnumeratorPtr GetChildren();
    db::RecordsetPtr GetInfo();
};

} // namespace mediatree

#endif
