#include "counted_object.h"

void intrusive_ptr_add_ref(util::CountedObject* o)
{
    o->m_refcount.fetch_add(1);
}

void intrusive_ptr_release(util::CountedObject* o)
{
    if (o->m_refcount.fetch_sub(1) == 1) {
	delete o;
    }
}

#ifdef TEST

# include "trace.h"
# include "counted_pointer.h"

static bool s_exists;

class Foo: public util::CountedObject
{
public:
    Foo() { s_exists = true; }
    ~Foo() { s_exists = false; }
};

static void Test()
{
    util::CountedPointer<Foo> fooptr(new Foo());

    {
	util::CountedPointer<Foo> fooptr2 = fooptr;
	{
	    util::CountedPointer<Foo> fooptr3 = fooptr2;

	    //TRACE << fooptr3 << "\n";
	}
    }

    fooptr.reset(NULL);

    assert(!s_exists);
}

int main()
{
    Test();
    return 0;
}

#endif
