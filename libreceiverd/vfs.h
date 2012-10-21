#ifndef LIBRECEIVERD_VFS_H
#define LIBRECEIVERD_VFS_H 1

#include <string>

namespace receiverd {

/** Virtual file system API.
 *
 * Not a very complete VFS: only enough for enough of a read-only NFS
 * server to boot a Rio Receiver.
 */
class VFS
{
public:
    virtual ~VFS() {}

    // These are deliberately the same as the NFS numbers
    enum ftype {
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5
    };

    virtual unsigned int Stat(unsigned int fh,
			      unsigned int *type, // e.g. NFREG
			      unsigned int *mode,
			      unsigned int *size,
			      unsigned int *mtime,
			      unsigned int *devt) = 0;
    virtual unsigned int GetHandleForName(const std::string& s,
					  unsigned int *fh) = 0;
    virtual unsigned int GetNameForHandle(unsigned int fh, std::string *s) = 0;
    virtual unsigned int ReadLink(unsigned int fh, std::string *s) = 0;
    virtual unsigned int Read(unsigned int fh, unsigned int offset,
			      void *buffer, unsigned int count, 
			      unsigned int *nread) = 0;
};

} // namespace receiverd

#endif
