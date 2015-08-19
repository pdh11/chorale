#include "cd_content_factory.h"
#include "libutil/printf.h"
#include "libutil/stream.h"
#include <stdio.h>

namespace import {

std::string CDContentFactory::GetPrefix()
{
    return util::SPrintf("/cd%u/", m_index);
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
	rs->body_source = m_audiocd->GetTrackStream(track);
	rs->content_type = "audio/L16";
	return true;
    }

    return false;
}

} // namespace import
