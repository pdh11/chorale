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

/** Like a much lighter-weight boost::bind
 */
template <class T, unsigned (T::*FN)()>
Callback Bind(T* t)
{
    return Callback((void*)t, &Callback_<T, FN>);
}

} // namespace util

#endif
