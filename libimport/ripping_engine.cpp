#include "ripping_engine.h"
#include "scsi.h"
#include "libutil/trace.h"
#include "libutil/errors.h"

namespace import {

RippingEngine::RippingEngine(ScsiTransport *transport, unsigned ripping_flags,
			     unsigned min_lba, unsigned max_lba)
    : m_transport(transport),
      m_ripping_flags(ripping_flags),
      m_min_lba(min_lba),
      m_max_lba(max_lba),
      m_buffer(new unsigned char[BUFSIZE])
{
    // Don't even bother with drives that can't return Q subchannel
    assert(m_ripping_flags & (USE_Q|USE_RAW));
}

RippingEngine::~RippingEngine()
{
}

unsigned RippingEngine::ReadSector(unsigned int lba, unsigned char *pcm)
{
    unsigned int retries = 0;

    for (;;)
    {
	unsigned rc = 0;
	if (m_sectors.empty())
	    rc = ReadSectors(lba);
	else if (m_sectors.begin()->first <= lba)
	{
	    sectors_t::iterator i = m_sectors.find(lba);
	    if (i != m_sectors.end()
		&& !i->second.any_errors)
	    {
		memcpy(pcm, i->second.pcm, 2352);
		m_sectors.erase(i);
		return 0;
	    }
	    if (++retries == 5)
	    {
		TRACE << "Giving up\n";
		return EIO;
	    }
	    rc = ReadSectors(lba - (retries*3));
	} 
	else
	{
	    // Must have missed it
	    unsigned offset = m_sectors.begin()->first - lba;
	    if (offset > lba)
		offset = lba;
	    rc = ReadSectors(lba - offset);
	}
	if (rc)
	    return rc;
    }
}

unsigned RippingEngine::ReadSectors(unsigned startlba)
{
    unsigned int size = GetSectorSize(m_ripping_flags);
    unsigned int n = BUFSIZE/size;
    if (n+startlba >= m_max_lba)
	n = m_max_lba - startlba;

    TRACE << "Reading sectors " << startlba << "..+" << n << "\n";
    ReadCD rcd(m_ripping_flags, startlba, n);
    unsigned rc = rcd.Submit(m_transport, m_buffer.get(), n*size);
    if (rc)
	return rc;

    for (unsigned int i=0; i<n; ++i)
    {
	const unsigned char *sector = m_buffer.get() + i*size;

	const unsigned char *q = sector + 2352;
	if (m_ripping_flags & USE_C2BB)
	    q += 196;
	else if (m_ripping_flags & USE_C2B)
	    q += 194;

	for (unsigned int j=0; j<16; ++j)
	    printf(" %02x", q[j]);
	printf("\n");

	unsigned int lba = q[7]*60*75 + q[8]*75 + q[9];

	TRACE << "Got sector " << lba << "\n";

	if (!lba)
	    lba = startlba+i;
	GotSector(lba, sector);
    }

    return 0;
}

unsigned RippingEngine::GotSector(unsigned int lba, const unsigned char *sector)
{
    sectors_t::iterator i = m_sectors.find(lba);
    if (i == m_sectors.end())
    {
	m_sectors.insert(std::make_pair(lba,Sector(sector, m_ripping_flags)));
    }
    return 0;
}

RippingEngine::Sector::Sector(const unsigned char *sector, unsigned ripping_flags)
{
    memcpy(pcm, sector, 2352);
    any_errors = 0;
    if (ripping_flags & (USE_C2B | USE_C2BB))
    {
	memcpy(errors, sector+2352, 194);
	for (unsigned int i=0; i<194; ++i)
	    any_errors |= errors[i];
    }
}

} // namespace import
