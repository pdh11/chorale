#ifndef LIBIMPORT_CD_CONTENT_FACTORY_H
#define LIBIMPORT_CD_CONTENT_FACTORY_H 1

#include "libutil/http_server.h"
#include "audio_cd.h"

namespace import {

class CDContentFactory: public util::http::ContentFactory
{
    AudioCDPtr m_audiocd;
    unsigned int m_index;

public:
    CDContentFactory(unsigned int index) : m_index(index) {}
    ~CDContentFactory() {}

    void SetAudioCD(AudioCDPtr cd) { m_audiocd = cd; }
    void ClearAudioCD() { m_audiocd = NULL; }

    std::string GetPrefix();

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request *rq,
		       util::http::Response *rs);
};

} // namespace import

#endif
