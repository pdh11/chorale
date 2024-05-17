#ifndef TV_DVB_RECORDING_H
#define TV_DVB_RECORDING_H 1

#include "dvb.h"
#include "recording.h"
#include "libutil/counted_pointer.h"
#include <string>

namespace util { class Stream; }
namespace util { class Scheduler; }
namespace util { class TaskQueue; }
namespace util { class WorkerThreadPool; }

namespace tv {

namespace dvb {

class Service;

class Recording: public tv::Recording
{
    util::Scheduler *m_scheduler;
    util::TaskQueue *m_disk_queue;
    Service *m_service;
    unsigned int m_channel;
    std::string m_filename;
    bool m_started;
    std::unique_ptr<util::Stream> m_input_stream;
    std::unique_ptr<util::Stream> m_output_stream;
    std::unique_ptr<util::Stream> m_buffer_stream;

public:
    Recording(util::Scheduler*, util::TaskQueue *disk_queue,
	      Service*, unsigned int channel, time_t start, time_t end,
	      const std::string& filename);

    // Being a Recording
    unsigned int Run() override;
};

typedef util::CountedPointer<Recording> RecordingPtr;

} // namespace dvb

} // namespace tv

#endif
