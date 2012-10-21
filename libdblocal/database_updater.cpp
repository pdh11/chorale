#include "config.h"
#include "database_updater.h"
#include "libmediadb/xml.h"
#include "libmediadb/schema.h"
#include "libutil/errors.h"
#include "libutil/trace.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <boost/scoped_array.hpp>

namespace db {
namespace local {

DatabaseUpdater::DatabaseUpdater(const std::string& loroot,
				 const std::string& hiroot,
				 db::Database *thedb, 
				 mediadb::Database *idallocator,
				 util::Scheduler *scheduler,
				 util::TaskQueue *queue,
				 const std::string& dbfilename)
    : m_notifier(import::FileNotifierTask::Create(scheduler)),
      m_file_scanner(loroot, hiroot, thedb, idallocator, queue,
		     m_notifier.get()),
      m_scanning(false),
      m_changed(false),
      m_database_filename(dbfilename),
      m_db(thedb)
{
    m_file_scanner.AddObserver(this);
    m_notifier->SetObserver(this);

    unsigned int rc = mediadb::ReadXML(thedb, dbfilename.c_str());
    
    if (rc)
    {
	TRACE << "Reading db returned " << rc << "\n";
	
	/// @bug Clear out any partial (bogus) results
    }

    m_scanning = true;
    m_file_scanner.StartScan();

    m_notifier->Init();
}

DatabaseUpdater::~DatabaseUpdater()
{
}

void DatabaseUpdater::OnChange()
{
    util::Mutex::Lock lock(m_mutex);
    if (m_scanning)
	m_changed = true;
    else
    {
	m_scanning = true;
	m_changed = false;
	m_file_scanner.StartScan();
    }
}

unsigned int DatabaseUpdater::OnFile(const std::string&)
{
    return 0;
}

void DatabaseUpdater::OnFinished(unsigned int)
{
#if HAVE_MKSTEMP
    boost::scoped_array<char> buffer(new char[m_database_filename.size() + 8]);
    sprintf(buffer.get(), "%s.XXXXXX", m_database_filename.c_str());
    int fd = mkstemp(buffer.get());

    FILE *f = fdopen(fd, "w");
    if (!f)
    {
	TRACE << "Can't write database: " << errno << "\n";
	close(fd);
    }
    else
    {
	unsigned int rc = mediadb::WriteXML(m_db, mediadb::SCHEMA_VERSION, f);
	if (rc)
	{
	    TRACE << "Can't write database: " << errno << "\n";
	    fclose(f);
	    ::unlink(buffer.get());
	    return;
	}

	fsync(fd);
	fclose(f); // Also closes the fd, see fdopen(3)

	::chmod(buffer.get(), 0644);

	::rename(buffer.get(), m_database_filename.c_str());
    }
#else
    /* No mkstemp (eg. Windows)
     */
    std::string name2 = m_database_filename + ".2";
    FILE *f = fopen(name2.c_str(), "w");

    if (!f)
    {
	TRACE << "Can't write database: " << errno << "\n";
    }
    else
    {
	mediadb::WriteXML(m_db, mediadb::SCHEMA_VERSION, f);
	fclose(f);

	::rename(name2.c_str(), m_database_filename.c_str());
    }
#endif

    util::Mutex::Lock lock(m_mutex);
    if (m_changed)
    {
	m_changed = false;
	m_scanning = true;
	m_file_scanner.StartScan();
    }
    else
	m_scanning = false;
}

void DatabaseUpdater::ForceRescan()
{
    OnChange();
}

} // namespace db::local
} // namespace db
