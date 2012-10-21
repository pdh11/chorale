#ifndef CHORALED_DATABASE_H
#define CHORALED_DATABASE_H 1

#include "config.h"
#include "libdbsteam/db.h"
#include "libdblocal/file_scanner_thread.h"
#include "libdblocal/db.h"

#define HAVE_LOCAL_DB HAVE_TAGLIB

namespace choraled {

class LocalDatabase
{
    db::steam::Database m_sdb;
    db::local::FileScannerThread m_ifs;
    db::local::Database m_ldb;

public:
    LocalDatabase();
    
    unsigned int Init(const std::string& loroot, const std::string& hiroot,
		      util::TaskQueue *queue, const std::string& dbfilename);

    db::local::Database *Get() { return &m_ldb; }

    void ForceRescan() { m_ifs.ForceRescan(); }
};

} // namespace choraled

#endif
