#ifndef LIBISAM_PAGE_ALLOCATOR_H
#define LIBISAM_PAGE_ALLOCATOR_H 1

#include "libutil/mutex.h"

namespace isam {

class File;

class PageAllocator
{
    File *m_file;
    util::Mutex m_mutex;

    static unsigned int FindUnused(const void *bitmap);
    static void MarkUsed(void *bitmap, unsigned int which);
    static void MarkUnused(void *bitmap, unsigned int which);

public:
    explicit PageAllocator(File *file);
    ~PageAllocator();

    unsigned int Allocate();
    void Free(unsigned int page);
};

} // namespace isam

#endif
