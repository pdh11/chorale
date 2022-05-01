#ifndef LIBIMPORT_RIPPING_ENGINE_H
#define LIBIMPORT_RIPPING_ENGINE_H

#include <map>
#include <boost/scoped_array.hpp>

namespace import {

class ScsiTransport;

class RippingEngine
{
    ScsiTransport *m_transport;

    unsigned m_ripping_flags;

    unsigned m_max_lba;

    struct Sector
    {
	unsigned char pcm[2352];
	unsigned char errors[2352/8];
	unsigned char any_errors;

	Sector(const unsigned char *sector, unsigned ripping_flags);
    };

    typedef std::map<unsigned, Sector> sectors_t;
    sectors_t m_sectors;

    enum { BUFSIZE = 128*1024 };

    boost::scoped_array<unsigned char> m_buffer;

    unsigned GotSector(unsigned int lba, const unsigned char *sector);

    unsigned ReadSectors(unsigned int lba);

public:
    RippingEngine(ScsiTransport *transport, unsigned ripping_flags,
		  unsigned min_lba, unsigned max_lba);
    ~RippingEngine();

    unsigned ReadSector(unsigned int lba, unsigned char *pcm);
};

} // namespace import

#endif
