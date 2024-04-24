#include "bind.h"

#ifdef TEST

#include <assert.h>
#include "counted_object.h"
#include "counted_pointer.h"

class Foo: public util::CountedObject
{
    static Foo *sm_this;
public:
    Foo() { sm_this = this; }

    unsigned int Bar() { assert(sm_this == this); return 42; }
    unsigned int Bark(const char*) { assert(sm_this == this); return 42; }
};

Foo *Foo::sm_this = NULL;

typedef util::CountedPointer<util::CountedObject> BasePtr;
typedef util::PtrCallback<BasePtr> BaseCallback;

typedef util::CountedPointer<Foo> FooPtr;
typedef util::PtrCallback<FooPtr> FooCallback;


class Handler
{
    util::Callback m_cb;
    util::Callback1<const char*> m_cbchar;
    BaseCallback m_fcb;

public:
    Handler(const util::Callback& cb,
	    const util::Callback1<const char*> cbchar,
	    const BaseCallback& fcb) 
	: m_cb(cb), m_cbchar(cbchar), m_fcb(fcb) {}

    unsigned int RunIt() { return m_cb() + m_cbchar("Hello") + m_fcb(); }
};

int main()
{
    util::CountedPointer<Foo> fooptr(new Foo);

    FooCallback fcb;
    BaseCallback bcb(fcb);

    Handler h(util::Bind(fooptr.get()).To<&Foo::Bar>(),
	      util::Bind(fooptr.get()).To<const char*,&Foo::Bark>(),
	      util::Bind(fooptr).To<&Foo::Bar>()
	);

    assert(h.RunIt() == 126);

    return 0;
}

#endif
