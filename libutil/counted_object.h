#ifndef COUNTED_OBJECT_H
#define COUNTED_OBJECT_H

#include <atomic>

namespace util {

class CountedObject;

} // namespace util

void intrusive_ptr_add_ref(util::CountedObject*);
void intrusive_ptr_release(util::CountedObject*);

namespace util {

class CountedObject
{
    std::atomic_uint m_refcount;

public:
    CountedObject() : m_refcount(0) {}
    virtual ~CountedObject() {}

    friend void ::intrusive_ptr_add_ref(CountedObject*);
    friend void ::intrusive_ptr_release(CountedObject*);
};

// Forward declaration -- see counted_pointer.h
template <class T> class CountedPointer;

} // namespace util

#endif
