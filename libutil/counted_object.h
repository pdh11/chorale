#ifndef COUNTED_OBJECT_H
#define COUNTED_OBJECT_H

#include <boost/intrusive_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include "not_thread_safe.h"

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

class SimpleCountedObject;

void intrusive_ptr_add_ref(SimpleCountedObject*);
void intrusive_ptr_release(SimpleCountedObject*);

class SimpleCountedObject: public util::NotThreadSafe
{
    unsigned int m_refcount;
public:
    SimpleCountedObject() : m_refcount(0) { CheckThread(); }
    virtual ~SimpleCountedObject() { CheckThread(); }

    friend void intrusive_ptr_add_ref(SimpleCountedObject*);
    friend void intrusive_ptr_release(SimpleCountedObject*);
};

#endif
