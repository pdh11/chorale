#include "file.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "libutil/file_stream.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

namespace isam {

File::File()
    : m_fd(-1)
{
}

File::~File()
{
    Close();
}

unsigned int File::Open(const char *fname, unsigned int)
{
    if (!fname)
	return 0;
    
    int rc = ::open(fname, O_RDWR|O_CREAT, 0600);
    if (rc < 0)
	return (unsigned)errno;
    m_fd = rc;
    return 0;
}

void *File::Page(unsigned int pageno)
{
//    TRACE << "Getting page " << pageno << "\n";

    unsigned int chunkno = pageno >> SHIFT;

    if (chunkno >= m_mb.size())
    {
	util::Mutex::Lock lock(m_mutex);

	if (chunkno >= m_mb.size())
	{
	    // Extend file first if necessary
	    if (m_fd > 0)
	    {
		struct stat st;
		fstat(m_fd, &st);
		if (st.st_size < (chunkno+1)*CHUNK)
		{
		    int rc = ftruncate(m_fd, (chunkno+1)*CHUNK);
		    if (rc < 0)
		    {
			TRACE << "ftruncate failed " << errno << "\n";
			return NULL;
		    }
		}
	    }
	    
	    int flags = MAP_SHARED;
	    if (m_fd == -1)
		flags |= MAP_ANONYMOUS;
	    
	    while (chunkno >= m_mb.size())
	    {
		void *ptr = mmap(NULL, CHUNK,
				 PROT_READ|PROT_WRITE, 
				 flags, m_fd, m_mb.size() * CHUNK);
		if (!ptr)
		{
		    TRACE << "mmap failed " << errno << "\n";
		    return NULL;
		}
		else
		{
//		TRACE << "chunk " << m_mb.size() << " is at " << ptr << "\n";
		}

		m_mb.push_back(ptr);
	    }
	}
    }

    pageno &= MASK;

//    TRACE << "Returning page " << pageno << " on chunk " << chunkno << "\n";

    return (void*)((char*)(m_mb[chunkno]) + pageno*PAGE);
}

void File::WriteOut(unsigned int pageno)
{
    if (m_fd != -1)
    {
	msync(Page(pageno), PAGE, MS_ASYNC);
	sync_file_range(m_fd, pageno*4096ull, 4096, SYNC_FILE_RANGE_WRITE);
    }
}

unsigned int File::Close()
{
    for (unsigned int i=0; i<m_mb.size(); ++i)
    {
	void *ptr = m_mb[i];
	msync(ptr, CHUNK, MS_ASYNC);
	munmap(ptr, CHUNK);
    }
    m_mb.clear();

    if (m_fd != -1)
    {
	::close(m_fd);
	m_fd = -1;
    }

    return 0;
}

} // namespace isam

#ifdef TEST

void Write(isam::File *f)
{
    srand(42);

    for (unsigned int i=0; i<10000; ++i)
    {
	unsigned int pos = ((i^180811u) * 57793u) % 10000000u;
	unsigned int val = rand()*rand();
	pos &= ~3;
	unsigned int page = pos >> 12;
	unsigned int ix = (pos & 4095) >> 2;

//	TRACE << "Pos " << pos << " is offset " << ix << " on page " << page
//	      << "\n";

	int *ptr = (int*)f->Page(page);
	assert(ptr != NULL);
	ptr[ix] = val;
    }
}

void Read(isam::File *f)
{
    srand(42);

    for (unsigned int i=0; i<10000; ++i)
    {
	unsigned int pos = ((i^180811u) * 57793u) % 10000000u;
	unsigned int val = rand()*rand();
	pos &= ~3;
	unsigned int page = pos >> 12;
	unsigned int ix = (pos & 4095) >> 2;
	unsigned int *ptr = (unsigned int*)f->Page(page);
	assert(ptr != NULL);
	assert(ptr[ix] == val);
    }
}

int main()
{
    {
	isam::File f;
	unsigned int rc = f.Open("test.isam", util::WRITE);
	assert(rc == 0);

	Write(&f);
	Read(&f);

	rc = f.Close();
	assert(rc == 0);
    }

    {
	isam::File f;
	unsigned int rc = f.Open("test.isam", util::UPDATE);
	assert(rc == 0);
	Read(&f);

	rc = f.Close();
	assert(rc == 0);
    }

    {
	isam::File f;
	unsigned int rc = f.Open(NULL, 0);
	assert(rc == 0);

	Write(&f);
	Read(&f);

	rc = f.Close();
	assert(rc == 0);
    }

    return 0;
}

#endif
