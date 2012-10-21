#ifndef MEDIATREE_RESULTS_H
#define MEDIATREE_RESULTS_H 1

#include "libutil/counted_object.h"
#include <vector>
#include "node.h"

namespace db { class Database; };

namespace mediatree {

class Results: public Node
{
    db::Database *m_db;
    int m_field;
    std::string m_value;
    db::RecordsetPtr m_info;

    Results(db::Database*, int field, const std::string& value);

    std::vector<NodePtr> m_children;
    
public:
    virtual ~Results() {}

    static NodePtr Create(db::Database*, int field, const std::string& value);

    std::string GetName();
    bool IsCompound() { return true; }
    bool HasCompoundChildren();

    EnumeratorPtr GetChildren();
    db::RecordsetPtr GetInfo();
};

}; // namespace mediatree

#endif
