#include "bind.h"

#ifdef TEST

#include <assert.h>

class Foo
{
    static Foo *sm_this;
public:
    Foo() { sm_this = this; }

    unsigned int Bar() { assert(sm_this == this); return 42; }
    unsigned int Bark(const char*) { assert(sm_this == this); return 42; }
};

Foo *Foo::sm_this = NULL;

class Handler
{
    util::Callback m_cb;
    util::Callback1<const char*> m_cbchar;

public:
    Handler(const util::Callback& cb,
	    const util::Callback1<const char*> cbchar) 
	: m_cb(cb), m_cbchar(cbchar) {}

    unsigned int RunIt() { return m_cb() + m_cbchar("Hello"); }
};

int main()
{
    Foo foo;

    Handler h(util::Bind<Foo,&Foo::Bar>(&foo),
	      util::Bind1<const char*,Foo,&Foo::Bark>(&foo)
	);

    assert(h.RunIt() == 84);

    return 0;
}

#endif
