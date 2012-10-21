#ifndef COUNTED_OBJECT_H
#define COUNTED_OBJECT_H

#include <boost/intrusive_ptr.hpp>
#include "locking.h"

namespace util {

template <class LockingPolicy>
class CountedObject;

} // namespace util

template <class LP>
void intrusive_ptr_add_ref(util::CountedObject<LP>*);
template <class LP>
void intrusive_ptr_release(util::CountedObject<LP>*);

namespace util {

template <class LockingPolicy=PerObjectLocking>
class CountedObject: public LockingPolicy
{
    unsigned int m_refcount;
public:
    CountedObject() : m_refcount(0) {}
    virtual ~CountedObject() {}

    friend void intrusive_ptr_add_ref<>(CountedObject*);
    friend void intrusive_ptr_release<>(CountedObject*);
};

} // namespace util

#endif
