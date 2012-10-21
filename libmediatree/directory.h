#ifndef MEDIATREE_DIRECTORY_H
#define MEDIATREE_DIRECTORY_H 1

#include <vector>
#include "node.h"

namespace db { class Database; }

namespace mediatree {

class Directory: public Node
{
    db::Database *m_db;
    unsigned int m_id;
    db::RecordsetPtr m_info;

    Directory(db::Database*, unsigned int id);
    Directory(db::Database*, unsigned int id, db::RecordsetPtr);

    std::vector<NodePtr> m_children;
    
public:
    virtual ~Directory() {}

    static NodePtr Create(db::Database*, unsigned int id);
    static NodePtr Create(db::Database*, unsigned int id, db::RecordsetPtr);

    std::string GetName();
    bool IsCompound();
    bool HasCompoundChildren();

    EnumeratorPtr GetChildren();
    db::RecordsetPtr GetInfo();
};

} // namespace mediatree

#endif
