#ifndef CHORALED_DATABASE_H
#define CHORALED_DATABASE_H 1

#include "libdbsteam/db.h"
#include "libimport/file_scanner_thread.h"
#include "libmediadb/localdb.h"

#if defined(HAVE_LIBXMLPP) && defined(HAVE_TAGLIB)
#define HAVE_DB 1
#endif

class Database
{
    db::steam::Database m_sdb;
    import::FileScannerThread m_ifs;
    mediadb::LocalDatabase m_ldb;

public:
    Database();
    
    unsigned int Init(const std::string& loroot, const std::string& hiroot,
		      util::TaskQueue *queue, const std::string& dbfilename);

    mediadb::LocalDatabase *Get() { return &m_ldb; }

    void ForceRescan() { m_ifs.ForceRescan(); }
};

#endif
