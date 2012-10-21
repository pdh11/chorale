#ifndef CHORALED_DATABASE_H
#define CHORALED_DATABASE_H 1

#include "config.h"
#include "libdbsteam/db.h"
#include "libdblocal/file_scanner_thread.h"
#include "libdblocal/db.h"

#if defined(HAVE_TAGLIB)
#define HAVE_DB 1
#endif

class Database
{
    db::steam::Database m_sdb;
#ifdef HAVE_DB
    db::local::FileScannerThread m_ifs;
#endif
    db::local::Database m_ldb;

public:
    Database();
    
    unsigned int Init(const std::string& loroot, const std::string& hiroot,
		      util::TaskQueue *queue, const std::string& dbfilename);

    db::local::Database *Get() { return &m_ldb; }

#ifdef HAVE_DB
    void ForceRescan() { m_ifs.ForceRescan(); }
#endif
};

#endif
