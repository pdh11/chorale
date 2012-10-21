#include "tarfs.h"
#include "libutil/trace.h"
#include <errno.h>
#include <string.h>

namespace receiverd {

TarFS::TarFS(util::SeekableStreamPtr stm)
    : m_stm(stm),
      m_current_sector(1) ///< Won't match any real (512-byte) sector offset
{
    ScanStream();
}

unsigned int TarFS::LoadSector(unsigned int n)
{
    if (n == m_current_sector)
	return 0;

//    TRACE << "Loading sector " << n << "\n";

    m_current_sector = n;

    size_t nread;
    unsigned int rc = m_stm->ReadAt(m_sector, n, 512, &nread);
    if (rc != 0)
	return rc;
    if (nread != 512)
	return EIO;
    return 0;
}

/** Walk the tar file noting the locations and names of each contained file.
 */
void TarFS::ScanStream()
{
    unsigned int pos = 0;
    util::SeekableStream::pos64 len = m_stm->GetLength();
    
    while (pos < len)
    {
	unsigned int rc = LoadSector(pos);
	if (rc != 0)
	{
	    TRACE << "Can't scan tar file: " << rc << "\n";
	    return;
	}

	if (m_sector[0] == '\0')
	    return;
	
	m_sector[99] = '\0';
	std::string path(m_sector);
	m_filemap[pos] = path;
	m_revmap[path] = pos;

//	TRACE << pos << " = " << path << "\n";

	m_sector[135] = '\0';
	unsigned int size = (unsigned int)strtol(m_sector+124, NULL, 8);

	// Round up to whole sectors
	size = (size + 511) & (~511);

	pos += size + 512;
    }
}

unsigned int TarFS::Stat(unsigned int fh,
			 unsigned int *type, unsigned int *mode,
			 unsigned int *size, unsigned int *mtime,
			 unsigned int *devt)
{
//    TRACE << "tarfs stat(" << fh << ")\n";

    unsigned int rc = LoadSector(fh);
    if (rc != 0)
	return rc;

    // http://en.wikipedia.org/wiki/Tar_(file_format)

    switch (m_sector[156])
    {
    case 0:
    case '0':
	*type = NFREG;
	break;
    case '2':
	*type = NFLNK;
	break;
    case '3':
	*type = NFCHR;
	break;
    case '4':
	*type = NFBLK;
	break;
    case '5':
	*type = NFDIR;
	break;
    default:
	*type = NFNON;
	break;
    }
    
    m_sector[107] = '\0';
    *mode = (unsigned int)strtol(m_sector+100, NULL, 8);
    m_sector[135] = '\0';
    *size = (unsigned int)strtol(m_sector+124, NULL, 8);
    m_sector[147] = '\0';
    *mtime = (unsigned int)strtol(m_sector+136, NULL, 8);
    m_sector[336] = '\0';
    unsigned int major = (unsigned int)strtol(m_sector+329, NULL, 8);
    m_sector[344] = '\0';
    unsigned int minor = (unsigned int)strtol(m_sector+337, NULL, 8);
    *devt = (major << 8) | minor;

    return 0;
}

unsigned int TarFS::GetNameForHandle(unsigned int fh, std::string *s)
{
    map_t::const_iterator i = m_filemap.find(fh);
    if (i == m_filemap.end())
	return ENOENT;
    *s = i->second;
    return 0;
}

unsigned int TarFS::GetHandleForName(const std::string& s, unsigned int *fh)
{
    revmap_t::const_iterator i = m_revmap.find(s);
    if (i == m_revmap.end())
    {
	// Try again with trailing '/'
	std::string sslash(s + '/');
	i = m_revmap.find(sslash);
	if (i == m_revmap.end())
	    return ENOENT;
    }
    *fh = i->second;
    return 0;
}

unsigned int TarFS::ReadLink(unsigned int fh, std::string *s)
{
    unsigned int rc = LoadSector(fh);
    if (rc != 0)
	return rc;
    m_sector[256] = '\0';
    *s = std::string(m_sector + 157);
    return 0;
}

unsigned int TarFS::Read(unsigned int fh, unsigned int offset,
			 void *buffer, unsigned int count, 
			 unsigned int *nread)
{
//    TRACE << "Read(" << fh << ", " << offset << ", +" << count << ")\n";

    *nread = 0;

    while (count > 0)
    {
	unsigned int rc = LoadSector(fh + 512 + (offset & ~511));
	if (rc != 0)
	    return rc;
	unsigned int lump = std::min(512-(offset & 511), count);
	
	memcpy(buffer, m_sector + (offset & 511), lump);
	*nread += lump;
	count -= lump;
	offset += lump;
	buffer = ((char*)buffer) + lump;
    }
    return 0;
}

} // namespace receiverd
