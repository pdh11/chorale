#ifndef LIBUTIL_BIND_H
#define LIBUTIL_BIND_H 1

#include <stdio.h>

namespace util {

template <class T, unsigned (T::*FN)()>
unsigned Callback_(void *p)
{
    T *t = (T*)p;
    return (t->*FN)();
}

class Callback
{
    typedef unsigned (*pfn)(void*);

    void *m_ptr;
    pfn m_pfn;

public:
    Callback() :  m_ptr(NULL), m_pfn(NULL) {}
    Callback(void *ptr, pfn ppfn) : m_ptr(ptr), m_pfn(ppfn) {}

    unsigned operator()() const { return (*m_pfn)(m_ptr); }

    operator bool() const { return m_ptr != NULL; }

    bool operator==(const Callback& other)
    {
	return m_ptr == other.m_ptr && m_pfn == other.m_pfn;
    }
};

/* -- */

template <class A1, class T, unsigned (T::*FN)(A1)>
unsigned Callback1_(void *p, A1 a1)
{
    T *t = (T*)p;
    return (t->*FN)(a1);
}

template <class A1>
class Callback1
{
    typedef unsigned (*pfn)(void*, A1);

    void *m_ptr;
    pfn m_pfn;
    
public:
    Callback1() :  m_ptr(NULL), m_pfn(NULL) {}
    Callback1(void *ptr, pfn ppfn) : m_ptr(ptr), m_pfn(ppfn) {}

    unsigned operator()(A1 a1) const { return (*m_pfn)(m_ptr, a1); }

    operator bool() const { return m_ptr != NULL; }

    bool operator==(const Callback1<A1>& other)
    {
	return m_ptr == other.m_ptr && m_pfn == other.m_pfn;
    }
};

/* -- */

template <class A1, class A2, class T, unsigned (T::*FN)(A1,A2)>
unsigned Callback2_(void *p, A1 a1, A2 a2)
{
    T *t = (T*)p;
    return (t->*FN)(a1, a2);
}

template <class A1, class A2>
class Callback2
{
    typedef unsigned (*pfn)(void*, A1, A2);

    void *m_ptr;
    pfn m_pfn;
    
public:
    Callback2() :  m_ptr(NULL), m_pfn(NULL) {}
    Callback2(void *ptr, pfn ppfn) : m_ptr(ptr), m_pfn(ppfn) {}

    unsigned operator()(A1 a1, A2 a2) const { return (*m_pfn)(m_ptr, a1, a2); }

    operator bool() const { return m_ptr != NULL; }

    bool operator==(const Callback2<A1,A2>& other)
    {
	return m_ptr == other.m_ptr && m_pfn == other.m_pfn;
    }
};

/* -- */

template <class T>
class Binder
{
    T *m_t;

public:
    Binder(T *t) : m_t(t) {}

    template <unsigned (T::*FN)()>
    Callback To() 
    {
	return Callback((void*)m_t, &Callback_<T,FN>);
    }

    template <class A1, unsigned (T::*FN)(A1)>
    Callback1<A1> To()
    {
	return Callback1<A1>((void*)m_t, &Callback1_<A1,T,FN>);
    }

    template <class A1, class A2, unsigned (T::*FN)(A1,A2)>
    Callback2<A1,A2> To()
    {
	return Callback2<A1,A2>((void*)m_t, &Callback2_<A1,A2,T,FN>);
    }
};

/** Like a much lighter-weight boost::bind
 *
 * Use as Bind(ptr).To(&Ptr::Member)
 */
template <typename T>
Binder<T> Bind(T* t)
{
    return Binder<T>(t);
}

/** Like Callback, but for counted pointers */

template <class PTR>
class PtrCallback
{
    typedef unsigned (*pfn)(void*);

    PTR m_ptr;
    pfn m_pfn;

public:
    PtrCallback() :  m_ptr(NULL), m_pfn(NULL) {}
    PtrCallback(const PTR& ptr, pfn ppfn) : m_ptr(ptr), m_pfn(ppfn) {}

    // Type conversion if PTR supports type conversion
    template <class OTHERPTR>
    PtrCallback( const PtrCallback<OTHERPTR>& other)
	: m_ptr(other.GetPtr()), m_pfn(other.GetFn())
    {
    }

    unsigned operator()() const { return (*m_pfn)((void*)m_ptr.get()); }

    operator bool() const { return m_ptr; }

    bool operator==(const PtrCallback& other)
    {
	return m_ptr == other.m_ptr && m_pfn == other.m_pfn;
    }

    const PTR& GetPtr() const { return m_ptr; }
    pfn GetFn() const { return m_pfn; }
};

template <template <class X> class PTR, class T>
class PtrBinder
{
    const PTR<T>& m_t;

public:
    PtrBinder(const PTR<T>& t) : m_t(t) {}

    template <unsigned (T::*FN)()>
    PtrCallback<PTR<T> > To() 
    {
	return PtrCallback<PTR<T> >(m_t, &Callback_<T,FN>);
    }
};


/** Like a much lighter-weight boost::bind
 */
template <template <class X> class PTR, class T>
PtrBinder<PTR,T> Bind(const PTR<T>& tptr)
{
    return PtrBinder<PTR,T>(tptr);
}

} // namespace util

#endif
