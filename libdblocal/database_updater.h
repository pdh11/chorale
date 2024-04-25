#ifndef LIBDBLOCAL_DATABASE_UPDATER_H
#define LIBDBLOCAL_DATABASE_UPDATER_H 1

#include "libimport/file_notifier.h"
#include "libutil/counted_pointer.h"
#include "libutil/mutex.h"
#include "file_scanner.h"

namespace db {

namespace local {

class DatabaseUpdater: public import::FileNotifierTask::Observer,
		       public FileScanner::Observer
{
    import::FileNotifierPtr m_notifier;
    FileScanner m_file_scanner;
    util::Mutex m_mutex;
    bool m_scanning;
    bool m_changed;
    std::string m_database_filename;
    db::Database *m_db;

    // Being a FileScanner::Observer
    unsigned int OnFile(const std::string& filename);
    void OnFinished(unsigned int error);

    // Being a util::FileNotifierTask::Observer
    void OnChange();

public:
    DatabaseUpdater(const std::string& loroot, const std::string& hiroot,
		    db::Database *thedb, mediadb::Database *idallocator,
		    util::Scheduler *scheduler, util::TaskQueue *queue, 
		    const std::string& dbfilename);
    ~DatabaseUpdater();

    void ForceRescan();
};

} // namespace db::local
} // namespace db

#endif
