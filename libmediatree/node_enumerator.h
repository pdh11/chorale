#ifndef MEDIATREE_NODE_ENUMERATOR_H
#define MEDIATREE_NODE_ENUMERATOR_H 1

#include "node.h"
#include <string>

namespace mediatree {

template <class T>
class ContainerEnumerator: public Node::Enumerator
{
    typename T::const_iterator m_iter;
    typename T::const_iterator m_end;

public:
    ContainerEnumerator(const T& t)
	: m_iter(t.begin()), 
	  m_end(t.end())
    {}

    bool IsValid() { return m_iter != m_end; }
    NodePtr Get() { return *m_iter; }
    void Next() { ++m_iter; }
};

} // namespace mediatree

#endif
