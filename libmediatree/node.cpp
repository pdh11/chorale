#include "node.h"
#include "libutil/counted_pointer.h"

namespace mediatree {

bool Node::IsCompound()
{
    return GetChildren()->IsValid(); 
}

} // namespace mediatree
