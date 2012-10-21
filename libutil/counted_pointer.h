#ifndef COUNTED_POINTER_H
#define COUNTED_POINTER_H 1

#include <stddef.h>
#include <assert.h>

namespace util {

template <class T>
class CountedPointer
{
    T *m_ptr;

public:
    CountedPointer()
	: m_ptr(NULL)
    {
    }

    explicit CountedPointer(T *ptr)
	: m_ptr(ptr)
    {
	if (ptr)
	    intrusive_ptr_add_ref(ptr);
    }

    CountedPointer(const CountedPointer& other)
	: m_ptr(other.m_ptr)
    {
	if (m_ptr)
	    intrusive_ptr_add_ref(m_ptr);
    }

    template <class T2>
    CountedPointer(const CountedPointer<T2>& other)
	: m_ptr(other ? other.get() : NULL)
    {
	if (m_ptr)
	    intrusive_ptr_add_ref(m_ptr);
    }

    ~CountedPointer()
    {
	if (m_ptr)
	    intrusive_ptr_release(m_ptr);
    }

    template <class T2>
    CountedPointer& operator=(const CountedPointer<T2>& other)
    {
	if (other)
	{
	    T *ptr = other.get();
	    return reset(ptr);
	}
	else
	    return reset(NULL);
    }

    CountedPointer& operator=(const CountedPointer& other)
    {
	return reset(other.m_ptr);
    }

    CountedPointer& operator=(T *ptr)
    {
	return reset(ptr);
    }

    CountedPointer& reset(T *ptr)
    {
	if (ptr != m_ptr)
	{
	    if (m_ptr)
		intrusive_ptr_release(m_ptr);
	    m_ptr = ptr;
	    if (m_ptr)
		intrusive_ptr_add_ref(m_ptr);
	}
	return *this;
    }

    T *operator->()
    {
	assert(m_ptr);
	return m_ptr;
    }

    const T *operator->() const
    {
	assert(m_ptr);
	return m_ptr;
    }

    T *get() const
    {
	assert(m_ptr);
	return m_ptr;
    }

    bool operator==(const CountedPointer& other) const
    {
	return m_ptr == other.m_ptr;
    }

    bool operator!=(const CountedPointer& other) const
    {
	return m_ptr != other.m_ptr;
    }

    typedef T *CountedPointer::*convertible_to_bool;

    operator convertible_to_bool() const
    {
	return m_ptr ? &CountedPointer::m_ptr : NULL;
    }
};

} // namespace util

#endif
