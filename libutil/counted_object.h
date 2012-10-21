#ifndef COUNTED_OBJECT_H
#define COUNTED_OBJECT_H

#include <boost/intrusive_ptr.hpp>
#include <boost/thread/mutex.hpp>

class CountedObject;

void intrusive_ptr_add_ref(CountedObject*);
void intrusive_ptr_release(CountedObject*);

class CountedObject
{
    boost::mutex m_refcount_mutex;
    unsigned int m_refcount;
public:
    CountedObject() : m_refcount(0) {}
    virtual ~CountedObject() {}

    friend void intrusive_ptr_add_ref(CountedObject*);
    friend void intrusive_ptr_release(CountedObject*);
};

#endif
