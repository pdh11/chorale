#include "cd_content_factory.h"
#include <boost/format.hpp>

namespace import {

std::string CDContentFactory::GetPrefix()
{
    return (boost::format("/cd%u/") % m_index).str();
}

bool CDContentFactory::StreamForPath(const util::http::Request *rq, 
				     util::http::Response *rs)
{
    unsigned int index, track;
//    TRACE << "CDCF looks at path " << path << "\n";
    if (sscanf(rq->path.c_str(), "/cd%u/%u.pcm", &index, &track) == 2
	&& index == m_index
	&& m_audiocd)
    {
	rs->ssp = util::SeekableStreamPtr(m_audiocd->GetTrackStream(track));
	rs->content_type = "audio/L16";
	return true;
    }

    return false;
}

} // namespace import
