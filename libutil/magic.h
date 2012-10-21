#ifndef LIBUTIL_MAGIC_H
#define LIBUTIL_MAGIC_H

#include <assert.h>

namespace util {

template <unsigned int VAL, bool debug>
class MagicImpl;

template <unsigned int VAL>
class MagicImpl<VAL, true>
{
    unsigned int m_magic;

public:
    MagicImpl() : m_magic(VAL) {}
    ~MagicImpl() { m_magic = ~VAL; }

    void AssertValid() const { assert(m_magic == VAL); }
    bool IsValid() const { return m_magic == VAL; }
};

template <unsigned int VAL>
class MagicImpl<VAL, false>
{
public:
    void AssertValid() const {}
    bool IsValid() const { return true; }
};

/** Generic "magic number" support, for validating that a pointer is what it
 * ought to be.
 *
 * Disappears to nothing in release builds (DEBUG is defined to 0 or 1 on the
 * command-line).
 */
template <unsigned int VAL>
class Magic: public MagicImpl<VAL, DEBUG>
{
};

template <class T>
void AssertValid(const T *t) { t->AssertValid(); }

} // namespace util

#endif
