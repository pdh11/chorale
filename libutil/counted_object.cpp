#include "counted_object.h"

void intrusive_ptr_add_ref(CountedObject *o)
{
    boost::mutex::scoped_lock lock(o->m_refcount_mutex);
    ++o->m_refcount;
}

void intrusive_ptr_release(CountedObject *o)
{
    bool deleteit = false;
    {
	boost::mutex::scoped_lock lock(o->m_refcount_mutex);
	if (!--o->m_refcount)
	    deleteit = true;
    }
    if (deleteit)
	delete o;
}

void intrusive_ptr_add_ref(SimpleCountedObject *o)
{
    o->CheckThread();
    ++o->m_refcount;
}

void intrusive_ptr_release(SimpleCountedObject *o)
{
    o->CheckThread();
    if (!--o->m_refcount)
	delete o;
}
