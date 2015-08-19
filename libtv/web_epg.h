#ifndef TV_WEB_EPG_H
#define TV_WEB_EPG_H 1

#include <string>
#include "libutil/http_server.h"

namespace db { class Database; }

namespace tv {

class Recordings;

namespace dvb { class Service; }
namespace dvb { class Channels; }

/** Interactive Web front-end for EPG data
 */
class WebEPG: public util::http::ContentFactory
{
    db::Database *m_db;
    dvb::Channels *m_ch;
    dvb::Service *m_dvb_service;

    std::string FullItem(unsigned int id, const std::string&, 
			 const std::string&, unsigned int);

    std::unique_ptr<util::Stream> EPGStream(bool tv, bool radio, int day);
    std::unique_ptr<util::Stream> ExpandStream(unsigned int id);
    std::unique_ptr<util::Stream> RecordStream(unsigned int id);
    std::unique_ptr<util::Stream> CancelStream(unsigned int id);

public:
    WebEPG();

    unsigned int Init(util::http::Server*, db::Database*, dvb::Channels*,
		      dvb::Service*);

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request *rq, 
		       util::http::Response *rs) override;
};

} // namespace tv

#endif
