#ifndef LIBUTIL_BIND_H
#define LIBUTIL_BIND_H 1

#include <stdio.h>
#include "counted_pointer.h"

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

/** Like a much lighter-weight boost::bind
 */
template <class T, unsigned (T::*FN)()>
Callback Bind(T* t)
{
    return Callback((void*)t, &Callback_<T, FN>);
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
    PtrCallback(PTR ptr, pfn ppfn) : m_ptr(ptr), m_pfn(ppfn) {}

    unsigned operator()() const { return (*m_pfn)((void*)m_ptr.get()); }

    operator bool() const { return m_ptr; }

    bool operator==(const PtrCallback& other)
    {
	return m_ptr == other.m_ptr && m_pfn == other.m_pfn;
    }

    PTR GetPtr() const { return m_ptr; }
};

/** Like a much lighter-weight boost::bind
 */
template <class PTR, class T, unsigned (T::*FN)()>
PtrCallback<PTR> BindPtr(CountedPointer<T> t)
{
    return PtrCallback<PTR>(t, &Callback_<T, FN>);
}

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

template <class A1, class T, unsigned (T::*FN)(A1)>
Callback1<A1> Bind1(T* t)
{
    return Callback1<A1>((void*)t, &Callback1_<A1, T, FN>);
}

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

template <class A1, class A2, class T, unsigned (T::*FN)(A1,A2)>
Callback2<A1,A2> Bind2(T* t)
{
    return Callback2<A1,A2>((void*)t, &Callback2_<A1, A2, T, FN>);
}

} // namespace util

#endif
