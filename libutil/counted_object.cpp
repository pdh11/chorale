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
    {
	delete o;
    }
}

/* Explicit instantiations (note "template" not "template<>") */

template 
void intrusive_ptr_add_ref<util::PerObjectRecursiveLocking>(util::CountedObject<util::PerObjectRecursiveLocking>*);
template 
void intrusive_ptr_release<util::PerObjectRecursiveLocking>(util::CountedObject<util::PerObjectRecursiveLocking>*);

template 
void intrusive_ptr_add_ref<util::PerObjectLocking>(util::CountedObject<util::PerObjectLocking>*);
template 
void intrusive_ptr_release<util::PerObjectLocking>(util::CountedObject<util::PerObjectLocking>*);

template void intrusive_ptr_add_ref<util::NoLocking>(util::CountedObject<util::NoLocking>*);
template void intrusive_ptr_release<util::NoLocking>(util::CountedObject<util::NoLocking>*);

#ifdef TEST

# include "trace.h"
# include "counted_pointer.h"

static bool s_exists;

template <class LockingPolicy>
class Foo: public util::CountedObject<LockingPolicy>
{
public:
    Foo() { s_exists = true; }
    ~Foo() { s_exists = false; }
};

template <class LockingPolicy>
void Test()
{
    util::CountedPointer<Foo<LockingPolicy> > fooptr(new Foo<LockingPolicy>());

    {
	util::CountedPointer<Foo<LockingPolicy> > fooptr2 = fooptr;
	{
	    util::CountedPointer<Foo<LockingPolicy> > fooptr3 = fooptr2;

	    TRACE << fooptr3 << "\n";
	}
    }

    fooptr.reset(NULL);
    
    assert(!s_exists);
}

int main()
{
    Test<util::NoLocking>();
    Test<util::PerObjectLocking>();
    Test<util::PerObjectRecursiveLocking>();
    Test<util::PerClassLocking<int> >();
    return 0;
}

#endif
