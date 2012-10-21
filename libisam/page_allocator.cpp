#include "page_allocator.h"
#include "file.h"

namespace isam {

PageAllocator::PageAllocator(File *file)
    : m_file(file)
{
}

PageAllocator::~PageAllocator()
{
}

void PageAllocator::MarkUsed(void *vbitmap, unsigned int which)
{
    unsigned char *ucbitmap = (unsigned char*)vbitmap;
    ucbitmap[which/8] = (unsigned char)(ucbitmap[which/8] | (1<< (which & 7)));
}

void PageAllocator::MarkUnused(void *vbitmap, unsigned int which)
{
    unsigned char *ucbitmap = (unsigned char*)vbitmap;
    ucbitmap[which/8] = (unsigned char)(ucbitmap[which/8] & ~(1 << (which & 7)));
}

unsigned int PageAllocator::FindUnused(const void *vbitmap)
{
    const unsigned char *ucbitmap = (const unsigned char*)vbitmap;

    for (unsigned int i=0; i<isam::PAGE; ++i)
    {
	unsigned char c = *ucbitmap++;
	if (c != 0xFF)
	{
	    unsigned int res = i*8;
	    do {
		if (!(c&1)) return res;
		c = (unsigned char)(c>>1);
		++res;
	    } while (1);
	}
    }
    return (unsigned int)-1;
}

unsigned int PageAllocator::Allocate()
{
    void *bitmap = m_file->Page(1);

    util::Mutex::Lock lock(m_mutex);

    unsigned int page = FindUnused(bitmap);
    while (page <= 1)
    {
	MarkUsed(bitmap, page);
	page = FindUnused(bitmap);
    }

    unsigned int offset = 0;

    while (page == (unsigned int)-1)
    {
	offset += 4096*8;
	bitmap = m_file->Page(offset);

	if (!bitmap)
	    return (unsigned int)-1;
	
	page = FindUnused(bitmap);
	if (!page)
	{
	    MarkUsed(bitmap, 0);
	    page = FindUnused(bitmap);
	}
    }

    MarkUsed(bitmap, page);
    return page + offset;
}

void PageAllocator::Free(unsigned int page)
{
    util::Mutex::Lock lock(m_mutex);

    if (page < 4096*8)
    {
	void *bitmap = m_file->Page(1);
	MarkUnused(bitmap, page);
	return;
    }

    void *bitmap = m_file->Page((page / (4096*8)) * 4096*8);
    MarkUnused(bitmap, page % (4096*8));
}

} // namespace isam

#ifdef TEST

int main()
{
    isam::File file;

    unsigned int rc = file.Open(NULL, 0);
    assert(rc == 0);

    isam::PageAllocator pa(&file);

    unsigned int page = pa.Allocate();
    assert(page == 2);
    page = pa.Allocate();
    assert(page == 3);
    pa.Free(2);
    page = pa.Allocate();
    assert(page == 2);

    return 0;
}

#endif
