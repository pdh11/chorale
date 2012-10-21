#ifndef IMPORT_FILE_SCANNER_H
#define IMPORT_FILE_SCANNER_H 1

#include <string>

namespace util { class TaskQueue; }

namespace mediadb { class Database; }

/** Classes for reading media files and their metadata.
 *
 * Including files and CDs.
 */
namespace import {

class FileNotifier;

class FileScanner
{
    class Impl;
    Impl *m_impl;

public:
    FileScanner(const std::string& loroot, const std::string& hiroot,
		mediadb::Database *thedb, util::TaskQueue *queue, 
		FileNotifier *fn = NULL);
    ~FileScanner();

    unsigned int Scan();
};

} // namespace import

#endif
