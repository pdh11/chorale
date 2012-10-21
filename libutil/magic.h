#ifndef LIBUTIL_MAGIC_H
#define LIBUTIL_MAGIC_H

#include <assert.h>

namespace util {

template <unsigned int VAL>
class Magic
{
    unsigned int m_magic;

public:
    Magic() : m_magic(VAL) {}
    ~Magic() { m_magic = ~VAL; }

    void AssertValid() const { assert(m_magic == VAL); }
    bool IsValid() const { return m_magic == VAL; }
};

template <class T>
void AssertValid(const T *t) { t->AssertValid(); }

}; // namespace util

#endif
