#include "libutil/worker_thread_pool.h"
#include "libutil/counted_pointer.h"
#include "libutil/walker.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/mutex.h"
#include "libutil/errors.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>

#ifdef __linux__

/* Same partition, 1202Mbytes

cp -dpR:

real    5m45.407s
user    0m0.184s
sys     0m11.837s

tar | tar:

real    5m42.446s
user    0m0.464s
sys     0m13.049s

tcp:

real    5m32.549s
user    0m0.420s
sys     0m13.333s

*/

class Copier: public util::DirectoryWalker::Observer
{
    util::TaskQueue *m_queue;
    enum { 
	LUMP = 8 * 1024 * 1024,
	QUEUE_MAX_BYTES = 32 * 1024 * 1024,
	QUEUE_MAX_TASKS = 256 // Too many and we get in EMFILE trouble (1024?)
    };
    util::Mutex m_mutex;
    util::Condition m_notfull;
    util::Condition m_finished;
    unsigned int m_inflight_bytes;
    uint64_t m_total_copied;
    struct timeval m_start;
    /** Number of running tasks (plus 1 if dir scan hasn't finished */
    unsigned int m_tasks;

public:
    Copier(util::TaskQueue *queue) 
	: m_queue(queue), m_inflight_bytes(0), m_total_copied(0), m_tasks(1)
    {
	gettimeofday(&m_start, NULL);
    }

    // Being a DirectoryWalker::Observer
    unsigned int OnFile(dircookie parent_cookie,
			unsigned int index,
			const std::string& path, 
			const std::string& leaf,
			const struct stat *st);
    unsigned int OnEnterDirectory(dircookie parent_cookie,
				  unsigned int index,
				  const std::string& path,
				  const std::string& leaf,
				  const struct stat *st,
				  dircookie *cookie_out);
    unsigned int OnLeaveDirectory(dircookie cookie,
				  const std::string& path,
				  const std::string& leaf,
				  const struct stat *st);
    void OnFinished(unsigned int error);

    // Callback for when writing has occurred
    void Wrote(unsigned int n);

    void WaitForCompletion();
};

class FileWriteTask: public util::Task
{
    Copier *m_copier;
    void *m_buffer;
    uint64_t m_offset;
    unsigned int m_len;
    int m_infd;
    int m_outfd;
    bool m_close;
    struct ::stat m_st;
    std::string m_path;

public:
    FileWriteTask(Copier *copier, void *buffer, uint64_t offset,
		  unsigned int len, int infd, int outfd,
		  bool close, const struct stat *st, const char *path)
	: m_copier(copier), m_buffer(buffer), m_offset(offset), m_len(len),
	  m_infd(infd), m_outfd(outfd), m_close(close), m_st(*st),
	  m_path(path) {}
    ~FileWriteTask() { m_copier->Wrote(m_len); }

    // Being a Task
    unsigned int Run();
};

time_t last = 0;

unsigned int FileWriteTask::Run()
{
    unsigned char *ptr = (unsigned char*)m_buffer;
    unsigned int len = m_len;
    off_t offset = m_offset;

    if (len)
    {
//	TRACE << "Writing " << len << " bytes at " << offset << "\n";

	unsigned int remainder = len % 4096;

	len = len - remainder; // Splice page-boundaries only

	if (len)
	{
	    int pipefd[2];

	    int rc = pipe(pipefd);
	    if (rc < 0)
	    {
		TRACE << "Pipe error " << errno << "\n";
		return errno;
	    }

	    do {
		off_t offset2 = offset;
		
//		TRACE << "Splicing-in " << len << " bytes at " << offset
//		      << "\n";
		ssize_t nread = splice(m_infd, &offset, pipefd[1], NULL, len,
				       SPLICE_F_MOVE);
//		TRACE << "Splice returned " << nread << "\n";
		if (nread <= 0)
		{
		    if (errno == EINTR)
			continue;

		    TRACE << "Read error " << errno << "\n";
		    close(pipefd[0]);
		    close(pipefd[1]);
		    return errno;
		}
		
		unsigned int len2 = (unsigned int)nread;

		while (len2 > 0)
		{
//		    TRACE << "Splicing-out " << len2 << " bytes at " << offset2
//			  << "\n";
		    ssize_t nwrote = splice(pipefd[0], NULL, 
					    m_outfd, &offset2, 
					    len2,
					    SPLICE_F_MOVE);
//		    TRACE << "Splice returned " << nwrote << "\n";
		    if (nwrote <= 0)
		    {
			if (errno == EINTR)
			    continue;
		    
			TRACE << "Write error " << errno << "\n";
			close(pipefd[0]);
			close(pipefd[1]);
			return errno;
		    }
		    len2 -= (unsigned int)nwrote;
		}
		
		len -= (unsigned int)nread;
	    } while (len);

	    close(pipefd[0]);
	    close(pipefd[1]);
	}

	if (remainder)
	{
//	    TRACE << "Mopping-up " << remainder << " bytes at " << offset+len
//		  << "\n";
	    unsigned char buffer[4096];
	    off_t rc = ::lseek(m_infd, offset+len, SEEK_SET);
//	    TRACE << "lseek() returned " << rc << "\n";
	    ssize_t nread = ::read(m_infd, buffer, remainder);
	    if (nread != remainder)
	    {
		TRACE << "Read " << nread << " error " << errno << "\n";
		return errno;
	    }
	    ssize_t nwrote = pwrite(m_outfd, buffer, remainder, offset+len);
	    if (nwrote != remainder)
	    {
		TRACE << "Write error " << errno << "\n";
		return errno;
	    }
	}

	sync_file_range(m_outfd, m_offset, m_len, SYNC_FILE_RANGE_WRITE);
    }

    if (ptr)
	munmap(ptr, m_len);

    if (m_close)
    {
	if (m_len)
	{
//	    sync_file_range(m_outfd, 0, m_offset+m_len,
//			    SYNC_FILE_RANGE_WRITE | SYNC_FILE_RANGE_WAIT_AFTER);
	    posix_fadvise(m_infd, 0, m_st.st_size, POSIX_FADV_DONTNEED);
	    posix_fadvise(m_outfd, 0, m_st.st_size, POSIX_FADV_DONTNEED);
	}
	int rc = fchown(m_outfd, m_st.st_uid, m_st.st_gid);
	fchmod(m_outfd, m_st.st_mode);
	struct timeval times[2];
	memset(times, '\0', sizeof(times));
	times[0].tv_sec = m_st.st_atim.tv_sec;
	times[1].tv_sec = m_st.st_mtim.tv_sec;
	futimes(m_outfd, times);
	close(m_outfd);
	close(m_infd);
    }

    return 0;
}

unsigned int Copier::OnEnterDirectory(dircookie, unsigned int,
				      const std::string& path,
				      const std::string&, const struct stat*,
				      dircookie*)
{
    std::string outfile = "/tmp" + path;
//    TRACE << "mkdir(" << outfile << ")\n";
    mkdir(outfile.c_str(), 0700);
    return 0;
}

unsigned int Copier::OnLeaveDirectory(dircookie, const std::string& path,
				      const std::string&,
				      const struct stat *st)
{
    std::string outfile = "/tmp" + path;
    chmod(outfile.c_str(), st->st_mode);
    return 0;
}

unsigned int Copier::OnFile(dircookie,
			    unsigned int,
			    const std::string& path, 
			    const std::string&,
			    const struct stat *st)
{
    std::string outfile = "/tmp" + path;

    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
    {
	int rc = mknod(outfile.c_str(), st->st_mode, st->st_rdev);
	if (rc < 0)
	{
	    TRACE << "Can't mknod " << outfile << ": " << errno << "\n";
	    return errno;
	}

	chown(outfile.c_str(), st->st_uid, st->st_gid);
	return 0;
    }

    int infd = open(path.c_str(), O_RDONLY);
    if (infd < 0)
    {
	TRACE << "Can't open " << path << ": " << errno << "\n";
	return errno;
    }

    posix_fadvise(infd, 0, st->st_size, POSIX_FADV_SEQUENTIAL);

    int outfd = open(outfile.c_str(), O_WRONLY|O_CREAT, 0600);
    if (outfd < 0)
    {
	TRACE << "Can't open " << outfile << ": " << errno << "\n";
	return errno;
    }
    
    ftruncate(outfd, st->st_size);
    posix_fadvise(outfd, 0, st->st_size, POSIX_FADV_SEQUENTIAL);
    fchmod(outfd, 0600);

    uint64_t len = st->st_size;
    uint64_t offset = 0;

    do {
	unsigned int lump = LUMP;
	if (lump > len)
	    lump = (unsigned int)len;

	// Wait until queue is < QUEUE_MAX bytes
	{
	    util::Mutex::Lock lock(m_mutex);
	    while ((m_inflight_bytes + lump >= QUEUE_MAX_BYTES)
		   || (m_tasks > QUEUE_MAX_TASKS))
	    {
		m_notfull.Wait(lock, 60);
	    }
	    m_inflight_bytes += lump;

	    time_t t = ::time(NULL);
	    if (t != last)
	    {
		last = t;
		printf("%u/%u %u/%u\n", m_inflight_bytes, QUEUE_MAX_BYTES,
		       m_tasks, QUEUE_MAX_TASKS);
	    }

	    ++m_tasks;
	}

//	TRACE << "Mapping " << lump << " bytes of " << path << "\n";

	void *ptr = NULL;

	if (lump)
	{
	    ptr = mmap(NULL, lump, PROT_READ, MAP_SHARED|MAP_POPULATE, infd, 
		       offset); // Forces read-in
	}

	len -= lump;

	util::CountedPointer<FileWriteTask> task(
	    new FileWriteTask(this, ptr, offset, lump, infd,
			      outfd, len==0, st, path.c_str()));
	m_queue->PushTask(util::Bind(task).To<&FileWriteTask::Run>());

	offset += lump;
    } while (len);

    return 0;
}

void Copier::Wrote(unsigned int n)
{
    bool doit = false;
    {
	util::Mutex::Lock lock(m_mutex);
	m_total_copied += n;
	m_inflight_bytes -= n;

	--m_tasks;

	time_t t = ::time(NULL);
	if (t != last)
	{
	    last = t;
	    printf("%u/%u %u/%u\n", m_inflight_bytes, QUEUE_MAX_BYTES,
		   m_tasks, QUEUE_MAX_TASKS);
	}
    
	if (m_inflight_bytes < QUEUE_MAX_BYTES && m_tasks < QUEUE_MAX_TASKS)
	    doit = true;
    }

    if (m_tasks == 0)
	m_finished.NotifyAll();

    if (doit)
	m_notfull.NotifyAll();
}

void Copier::OnFinished(unsigned int error)
{
    {
	util::Mutex::Lock lock(m_mutex);
	--m_tasks;

	if (m_tasks == 0)
	    m_finished.NotifyAll();
    }

    if (error)
	fprintf(stderr, "Scan failed: %u\n", error);
}

void Copier::WaitForCompletion()
{
    {
	util::Mutex::Lock lock(m_mutex);
	while (m_tasks)
	{
	    m_finished.Wait(lock, 60);
	}
    }
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t us = (now.tv_sec * 1000000LL + now.tv_usec)
	- (m_start.tv_sec * 1000000LL + m_start.tv_usec);
    if (us > 100000)
    {
	uint64_t bps = m_total_copied * 1000000LL / us;
	printf("%uMbytes copied in %u.%03us, %uMbytes/s\n",
	       (unsigned int)(m_total_copied >> 20),
	       (unsigned int)(us / 1000000),
	       (unsigned int)((us/1000) % 1000),
	       (unsigned int)(bps >> 20));
    }

    if (0)//error)
    {
//	fprintf(stderr, "Copy failed: %u\n", error);
//	exit(1);
    }
    printf("Copy succeeded\n");
}

#endif /* __linux__ */

int main(int argc, char *argv[])
{
#ifdef __linux__
    util::WorkerThreadPool writers(util::WorkerThreadPool::NORMAL, 1);
    util::WorkerThreadPool readers(util::WorkerThreadPool::NORMAL, 64);

    Copier tcp(&writers);
 
    std::string tmproot("/tmp");
    tmproot += argv[1];
    tmproot += "/foo";
    util::MkdirParents(tmproot.c_str());

    util::DirectoryWalker::Walk(argv[1], &tcp, &readers,
				util::DirectoryWalker::REPORT_LINKS | util::DirectoryWalker::ONE_FILESYSTEM);

    tcp.WaitForCompletion();
#endif

    return 0;
}
