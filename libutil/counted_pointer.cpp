#include "counted_pointer.h"
#include "counted_object.h"
#include <assert.h>

#ifdef TEST

class Foo: public util::CountedObject
{
};

typedef util::CountedPointer<Foo> FooPtr;

class Bar1: public Foo
{
};

typedef util::CountedPointer<Bar1> Bar1Ptr;

class Bar2: public Foo
{
};

typedef util::CountedPointer<Bar2> Bar2Ptr;

int main()
{
    FooPtr foop(new Foo);
    FooPtr foop2(new Foo);

    FooPtr foop1a(foop);

    assert(foop == foop1a);
    assert(foop != foop2);

    Bar1Ptr barp(new Bar1);
    Bar1Ptr barp1a(barp);
    Bar1Ptr barp2(new Bar1);

    FooPtr foop3(barp);

    const Bar1Ptr& cbp(barp);

    assert(barp == barp1a);
    assert(barp == cbp);
    assert(cbp == barp);
    assert(barp2 != cbp);
    assert(cbp != barp2);

    Bar1Ptr bar0; // empty
    assert(bar0 != barp);
    assert(barp != bar0);
    assert(cbp != bar0);
    assert(bar0 != cbp);

    return 0;
}

#endif
