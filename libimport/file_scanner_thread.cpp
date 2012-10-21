#include "file_scanner_thread.h"
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "file_scanner.h"
#include "file_notifier.h"
#include "libmediadb/xml.h"
#include "libmediadb/db.h"
#include "libutil/trace.h"

namespace import {

class FileScannerThread::Impl: public FileNotifier::Observer
{
    util::Poller m_poller;
    util::PollWaker m_waker;
    FileNotifier m_notifier;
    db::Database *m_db;
    FileScanner m_scanner;
    std::string m_dbfilename;
    volatile bool m_exiting;
    volatile bool m_changed;
    boost::mutex m_changed_mutex;
    boost::thread m_thread;

    void Run();

    void ScanAndRewrite();

    // Being a FileNotifier::Observer
    void OnChange();

public:
    Impl(const std::string& loroot, const std::string& hiroot,
	 mediadb::Database *thedb, util::TaskQueue *queue,
	 const std::string& dbfilename);
    ~Impl();

    void ForceRescan();
};

FileScannerThread::Impl::Impl(const std::string& loroot,
			      const std::string& hiroot,
			      mediadb::Database *thedb,
			      util::TaskQueue *queue,
			      const std::string& dbfilename)
    : m_waker(&m_poller, NULL),
      m_db(thedb),
      m_scanner(loroot, hiroot, thedb, queue, &m_notifier),
      m_dbfilename(dbfilename),
      m_exiting(false),
      m_changed(false),
      m_thread(boost::bind(&Impl::Run, this))
{
}

FileScannerThread::Impl::~Impl()
{
    m_exiting = true;
    m_waker.Wake();
    m_thread.join();
}

void FileScannerThread::Impl::ScanAndRewrite()
{
    struct timeval begin, end;

    TRACE << "scanning\n";

    gettimeofday(&begin, NULL);
    m_scanner.Scan();
    gettimeofday(&end, NULL);

    int64_t beginus = (begin.tv_sec * 1000000LL) + begin.tv_usec;
    int64_t endus = (end.tv_sec * 1000000LL) + end.tv_usec;
    int64_t elapsed = endus-beginus;

    char buf[80];
    sprintf(buf, "%llu.%06llu", elapsed/1000000ULL, elapsed%1000000ULL);

    TRACE << "Scan took " << buf << "s, writing\n";

    char *buffer = new char[m_dbfilename.size() + 8];
    sprintf(buffer, "%s.XXXXXX", m_dbfilename.c_str());
    int fd = mkstemp(buffer);

    FILE *f = fdopen(fd, "w");
    if (!f)
    {
	TRACE << "Can't write database: " << errno << "\n";
    }
    else
    {
	mediadb::WriteXML(m_db, 1, f);
	fclose(f);

	::rename(buffer, m_dbfilename.c_str());
    }

    delete[] buffer;
}

void FileScannerThread::Impl::Run()
{
    m_notifier.SetObserver(this);
    m_notifier.Init(&m_poller);

    setpriority(PRIO_PROCESS, 0, 15);

    ScanAndRewrite();

    while (!m_exiting)
    {
	m_poller.Poll(util::Poller::INFINITE_MS);

	bool do_scan = false;
	{
	    boost::mutex::scoped_lock lock(m_changed_mutex);
	    do_scan = m_changed;
	    m_changed = false;
	}
	if (do_scan)
	    ScanAndRewrite();
    }
}

void FileScannerThread::Impl::OnChange()
{
    boost::mutex::scoped_lock lock(m_changed_mutex);
    m_changed = true;
}

void FileScannerThread::Impl::ForceRescan()
{
    OnChange();
    m_waker.Wake();
}


        /* FileScannerThread */


FileScannerThread::FileScannerThread()
    : m_impl(NULL)
{
}

FileScannerThread::~FileScannerThread()
{
    delete m_impl;
}

unsigned int FileScannerThread::Init(const std::string& loroot,
				     const std::string& hiroot,
				     mediadb::Database *thedb,
				     util::TaskQueue *queue,
				     const std::string& dbfilename)
{    
    TRACE << "reading\n";

    mediadb::ReadXML(thedb, dbfilename.c_str());

    m_impl = new Impl(loroot, hiroot, thedb, queue, dbfilename);
    return 0;
}

void FileScannerThread::ForceRescan()
{
    if (m_impl)
	m_impl->ForceRescan();
}

} // namespace import

