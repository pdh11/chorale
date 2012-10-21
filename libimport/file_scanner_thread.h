#ifndef LIBIMPORT_FILE_SCANNER_THREAD_H
#define LIBIMPORT_FILE_SCANNER_THREAD_H

#include <string>

namespace util { class TaskQueue; }

namespace mediadb { class Database; }

namespace import {

class FileScannerThread
{
    class Impl;
    Impl *m_impl;
    
public:
    FileScannerThread();
    ~FileScannerThread();

    unsigned int Init(const std::string& loroot, const std::string& hiroot,
		      mediadb::Database *thedb, util::TaskQueue *queue,
		      const std::string& dbfilename);

    void ForceRescan();
};

} // namespace import

#endif
