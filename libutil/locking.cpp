#include "locking.h"

namespace util {

PerObjectLocking::PerObjectLocking()
{
}

PerObjectLocking::~PerObjectLocking()
{
}

PerObjectLocking::Lock::Lock(PerObjectLocking* l)
    : m_lock(l->m_mutex)
{
}

PerObjectLocking::Lock::~Lock()
{
}

PerObjectRecursiveLocking::PerObjectRecursiveLocking()
{
}

PerObjectRecursiveLocking::~PerObjectRecursiveLocking()
{
}

PerObjectRecursiveLocking::Lock::Lock(PerObjectRecursiveLocking* l)
    : m_lock(l->m_mutex)
{
}

PerObjectRecursiveLocking::Lock::~Lock()
{
}

} // namespace util
