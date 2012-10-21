#ifndef LIBISAM_PAGE_LOCKING_H
#define LIBISAM_PAGE_LOCKING_H 1

#include "libutil/mutex.h"
#include <set>
#include <deque>

namespace isam {

class File;

/** Locking of database pages (and waiting for locks).
 *
 * The rules to avoid deadlocks are as follows:
 *  - When holding a page lock, you MUST NOT lock any ancestor page, even if
 *    you also hold a lock on a page ancestral to that.
 *
 * Currently wake-ups pay no account to exactly *which* page has
 * become available: when any contended page comes free, all waiting
 * writers (or readers) get woken up, even if some were waiting for a
 * different page.
 */
class PageLocking
{
    /** Protects the multisets.
     *
     * There's a lot of contention on this mutex. Fix if needed by
     * having separate mutex+conditions+multisets structures per
     * (pageno%100), or some other size.
     */
    util::Mutex m_mutex;
    util::Condition m_writable;
    util::Condition m_readable;
    File *m_file;
    std::multiset<unsigned int> m_read_locked;
    std::set<unsigned int> m_write_locked;
    std::multiset<unsigned int> m_read_waiting;
    std::multiset<unsigned int> m_write_waiting;

    friend class ReadLock;
    friend class ReadLockChain;
    friend class WriteLock;
    friend class WriteLockChain;

    friend class DeleteLockChain;

    const void *LockForRead(unsigned int pageno);
    void UnlockForRead(unsigned int pageno);
    void *LockForWrite(unsigned int pageno);
    void UnlockForWrite(unsigned int pageno);

public:
    explicit PageLocking(File *file);
    ~PageLocking();

    File *GetFile() const { return m_file; }
};

class ReadLock
{
    PageLocking *m_locking;
    unsigned int m_pageno;

public:
    ReadLock(PageLocking *locking, unsigned int pageno)
	: m_locking(locking), m_pageno(pageno)
    {
    }

    const void *Lock() { return m_locking->LockForRead(m_pageno); }

    ~ReadLock()
    {
	m_locking->UnlockForRead(m_pageno);
    }
};

class WriteLock
{
    PageLocking *m_locking;
    unsigned int m_pageno;

public:
    WriteLock(PageLocking *locking, unsigned int pageno)
	: m_locking(locking), m_pageno(pageno)
    {
    }

    void *Lock() { return m_locking->LockForWrite(m_pageno); }

    ~WriteLock()
    {
	m_locking->UnlockForWrite(m_pageno);
    }
};

/** Models a chain of locks as a read lookup descends the hierarchy.
 *
 * Holds each lock while gaining the child lock, then releases it. Which is
 * enough for the no-deadlock rules. Except inside Add(), only holds one lock
 * at a time.
 */
class ReadLockChain
{
    PageLocking *m_locking;
    unsigned int m_page;

    unsigned int Add(unsigned int pageno);

public:
    explicit ReadLockChain(PageLocking*);
    ~ReadLockChain();

    unsigned int Add(unsigned int pageno, unsigned int)
    {
	return Add(pageno);
    }

    unsigned int GetChildPage() const { return m_page; }
};

/** Models a chain of locks as a write lookup descends the hierarchy.
 *
 * When rewriting a page, libisam creates a new version and then
 * atomically updates the parent. So when descending, we need to keep
 * two locks at all times: the page we're looking at, and its
 * parent. When we descend, on locking the grandchild page we can
 * release the lock on the grandparent page (and then shuffle them all
 * up one).
 */
class WriteLockChain
{
    PageLocking *m_locking;
    unsigned int m_parent_page;
    unsigned int m_child_page;

    unsigned int Add(unsigned int pageno);

public:
    explicit WriteLockChain(PageLocking*);
    ~WriteLockChain();

    unsigned int Add(unsigned int pageno, unsigned int)
    {
	return Add(pageno);
    }

    unsigned int GetParentPage() const { return m_parent_page; }
    unsigned int GetChildPage() const { return m_child_page; }
};

/** Models a chain of locks as a delete lookup descends the hierarchy.
 *
 * A delete is like a write, EXCEPT that count=0 pages aren't allowed,
 * so if our page is count=1 we must keep a write lock on all count=1
 * parents until we get to count>1 (or the root).
 */
class DeleteLockChain
{
    PageLocking *m_locking;
    std::deque<unsigned int> m_pages;

public:
    explicit DeleteLockChain(PageLocking*);
    ~DeleteLockChain();

    unsigned int Add(unsigned int pageno, unsigned int siblings);

    unsigned int GetParentPage() const { return m_pages.front(); }
    unsigned int GetChildPage() const { return m_pages.back(); }
    void ReleaseParentPage();
    void ReleaseChildPage();
    unsigned int Count() const { return (unsigned int)m_pages.size(); }
};

} // namespace isam

#endif
