#ifndef MEDIATREE_NODE_H
#define MEDIATREE_NODE_H 1

#include "libutil/counted_object.h"
#include <string>

namespace util { template <class T> class CountedPointer; }
namespace db { class Recordset; }

/** An abstract tree of items for accessing media, eg for use as a menu.
 */
namespace mediatree {

/** A node of the abstract media tree (e.g. "Artists", or a particular artist,
 * or a particular song).
 *
 * @todo Push the database pointer into Node? all the subclasses have one
 */
class Node: public util::CountedObject
{
public:
    virtual ~Node() {}

    virtual std::string GetName() = 0;
    virtual bool IsCompound();
    virtual bool HasCompoundChildren() = 0;

    typedef util::CountedPointer<Node> Pointer;

    class Enumerator: public util::CountedObject
    {
    public:
	virtual ~Enumerator() {}
	virtual bool IsValid() = 0;
	virtual Node::Pointer Get() = 0;
	virtual void Next() = 0;
    };

    typedef util::CountedPointer<Enumerator> EnumeratorPtr;

    virtual EnumeratorPtr GetChildren() = 0;
    virtual util::CountedPointer<db::Recordset> GetInfo() = 0;

    /** If node->GetValidity() != parent->GetValidity(), you need to call
     * parent->GetChildren() again.
     */
//    unsigned int GetValidity() = 0;

//    virtual unsigned int GetColumnSetId() ?
//    virtual uint64_t GetFieldBitmap() ?
};

typedef util::CountedPointer<Node> NodePtr;

} // namespace mediatree

#endif
