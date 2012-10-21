#ifndef LIBRECEIVERD_TARFS_H
#define LIBRECEIVERD_TARFS_H

#include "vfs.h"
#include <map>

namespace util { class Stream; }

namespace receiverd {

/** A virtual file system from the contents of a tar file.
 *
 * File handles are simply the offsets into the tar file of the file's header
 * block (see, for example, TarFS::Stat).
 */
class TarFS: public VFS
{
    util::Stream *m_stm;

    typedef std::map<unsigned int, std::string> map_t;
    map_t m_filemap;
    typedef std::map<std::string, unsigned int> revmap_t;
    revmap_t m_revmap;

    char m_sector[512];
    unsigned int m_current_sector;

    /** Note that n is the file offset (so must be a multiple of 512)
     */
    unsigned int LoadSector(unsigned int n);

    void ScanStream();

public:
    explicit TarFS(util::Stream*);

    unsigned int Stat(unsigned int fh,
		      unsigned int *type, unsigned int *mode,
		      unsigned int *size, unsigned int *mtime,
		      unsigned int *devt);
    unsigned int GetHandleForName(const std::string& s, unsigned int *fh);
    unsigned int GetNameForHandle(unsigned int fh, std::string *s);
    unsigned int ReadLink(unsigned int fh, std::string *s);
    unsigned int Read(unsigned int fh, unsigned int offset,
		      void *buffer, unsigned int count, 
		      unsigned int *nread);
};

} // namespace receiverd

#endif
