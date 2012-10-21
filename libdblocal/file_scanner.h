#ifndef IMPORT_FILE_SCANNER_H
#define IMPORT_FILE_SCANNER_H 1

#include <string>

namespace util { class TaskQueue; }

namespace import { class FileNotifierTask; }

namespace mediadb { class Database; }

namespace db {

class Database;

namespace local {

class FileScanner
{
    class Impl;
    Impl *m_impl;

public:
    class Observer
    {
    public:
	virtual ~Observer() {}
    
	/** WARNING this callback is made on multiple background threads.
	 *
	 * Return nonzero to abort the scan (OnFinished is still called).
	 */
	virtual unsigned int OnFile(const std::string& filename) = 0;

	/** WARNING this callback is made on a background thread.
	 *
	 * @param error The error that killed the scan (or 0 for success).
	 */
	virtual void OnFinished(unsigned int error) = 0;
    };

    /** Construct a FileScanner
     *
     * @param loroot      Root of lo-fi (MP3) music data
     * @param hiroot      Root of hi-fi (FLAC) music data
     * @param thedb       Underlying database
     * @param idallocator Media database responsible for new record IDs
     * @param queue       Task queue for media scanning tasks
     * @param fn          Optional FileNotifierTask
     *
     * All database accesses go via "thedb", except for allocating new
     * record IDs. This is a mediadb::Database method (because plain
     * databases don't know the mediadb schema which defines the ID
     * field), but the FileScanner can't just use the
     * mediadb::Database for everything, as it wraps recordset
     * operations to do re-tagging or file deletion.
     */
    FileScanner(const std::string& loroot, const std::string& hiroot,
		db::Database *thedb, mediadb::Database *idallocator, 
		util::TaskQueue *queue, import::FileNotifierTask *fn = NULL);
    ~FileScanner();

    void AddObserver(Observer*);
    void RemoveObserver(Observer*);

    unsigned int StartScan();
    unsigned int WaitForCompletion();

    unsigned int Scan();
};

} // namespace db::local
} // namespace db

#endif
