#include "counted_object.h"

template <class LockingPolicy>
void intrusive_ptr_add_ref(util::CountedObject<LockingPolicy> *o)
{
    typename LockingPolicy::Lock lock(o);
    ++o->m_refcount;
}

template <class LockingPolicy>
void intrusive_ptr_release(util::CountedObject<LockingPolicy> *o)
{
    bool deleteit = false;
    {
	typename LockingPolicy::Lock lock(o);
	if (!--o->m_refcount)
	    deleteit = true;
    }
    if (deleteit)
	delete o;
}

/* Explicit instantiations (note "template" not "template<>") */

template 
void intrusive_ptr_add_ref<util::PerObjectLocking>(util::CountedObject<util::PerObjectLocking>*);
template 
void intrusive_ptr_release<util::PerObjectLocking>(util::CountedObject<util::PerObjectLocking>*);

template void intrusive_ptr_add_ref<util::NoLocking>(util::CountedObject<util::NoLocking>*);
template void intrusive_ptr_release<util::NoLocking>(util::CountedObject<util::NoLocking>*);
