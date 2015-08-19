#ifndef LIBTV_DVB_SERVICE_H
#define LIBTV_DVB_SERVICE_H

#include <string>
#include <memory>
#include "libutil/locking.h"

namespace util { class Scheduler; }
namespace util { class TaskQueue; }
namespace util { class Stream; }
namespace tv {

namespace dvb {

class Frontend;
class Channels;

/** High-level interface to the complete DVB service.
 */
class Service: public util::PerObjectLocking
{
    Frontend *m_frontend;
    Channels *m_channels;
    std::string m_recordings_directory;
    util::Scheduler *m_poller;
    util::TaskQueue *m_disk_queue;

public:
    Service(Frontend*, Channels*, const std::string &directory,
	    util::Scheduler*, util::TaskQueue *disk_queue);
    ~Service();

    unsigned int Record(unsigned int channel, time_t start, time_t end,
			const std::string& title);

    unsigned int GetStream(unsigned int channel,
			   std::unique_ptr<util::Stream>*);
};

} // namespace dvb

} // namespace tv

#endif
