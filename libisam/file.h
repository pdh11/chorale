#ifndef LIBISAM_FILE_H
#define LIBISAM_FILE_H 1

#include <vector>
#include "libutil/mutex.h"

/** Classes for general key/value persistent storage.
 *
 * Note that it's indexed, it's sequential, and it's an access method,
 * but it's not actually the same as "the" famous Indexed Sequential
 * Access Method format used by large-scale databases.
 */
namespace isam {

enum { PAGE = 4096 };

class File
{
    int m_fd;
    util::Mutex m_mutex;
    std::vector<void*> m_mb;

    enum { CHUNK = 1024*1024 };

    enum { SHIFT = 20 - 12, // log2(CHUNK/PAGE)
	   MASK = 255       // CHUNK/PAGE-1
    };

public:
    File();
    ~File();

    /** Open a file, creating it if necessary.
     *
     * @arg fname Filename (pass NULL to get an in-memory file)
     * @arg mode  A util::FileMode (ignored if fname==NULL)
     */
    unsigned int Open(const char *fname, unsigned int mode);
    unsigned int Close();

    void *Page(unsigned int pageno);

    /** Starts asynchronous write-out of page.
     */
    void WriteOut(unsigned int pageno);
};

} // namespace isam

#endif
