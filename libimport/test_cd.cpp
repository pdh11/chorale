#include "test_cd.h"
#include "libutil/memory_stream.h"

namespace import {

TestCD::TestCD()
{
    static const TocEntry entries[] = {
	{ 0, 11 },
	{ 12, 100 },
	{ 101, 200 },
	{ 201, 600 },
	{ 601, 1200 },
	{ 1201, 1600 },
    };

    for (unsigned int i=0; i<6; ++i)
	m_toc.push_back(entries[i]);

    m_total_sectors = 1601;
}

std::auto_ptr<util::Stream> TestCD::GetTrackStream(unsigned int t)
{
    std::auto_ptr<util::Stream> ss(new util::MemoryStream);
    ss->SetLength((m_toc[t].last_sector - m_toc[t].first_sector + 1) * 2352);
    return ss;
}

TestCDDrive::TestCDDrive()
    : m_wtp(util::WorkerThreadPool::NORMAL, 1)
{
}

} // namespace import
