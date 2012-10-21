#ifndef MEDIATREE_DIRECTORY_H
#define MEDIATREE_DIRECTORY_H 1

#include <vector>
#include "node.h"
#include "libutil/counted_pointer.h"

namespace db { class Database; }

namespace mediatree {

class Directory: public Node
{
    db::Database *m_db;
    unsigned int m_id;
    util::CountedPointer<db::Recordset> m_info;

    Directory(db::Database*, unsigned int id);
    Directory(db::Database*, unsigned int id, util::CountedPointer<db::Recordset>);

    std::vector<NodePtr> m_children;
    
public:
    virtual ~Directory();

    static NodePtr Create(db::Database*, unsigned int id);
    static NodePtr Create(db::Database*, unsigned int id,
			  util::CountedPointer<db::Recordset>);

    std::string GetName();
    bool IsCompound();
    bool HasCompoundChildren();

    EnumeratorPtr GetChildren();
    util::CountedPointer<db::Recordset> GetInfo();
};

} // namespace mediatree

#endif
