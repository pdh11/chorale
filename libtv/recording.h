#ifndef TV_RECORDING_H
#define TV_RECORDING_H 1

#include "libutil/task.h"
#include <set>
#include <time.h>

namespace util { class Scheduler; }

namespace tv {

class Recording: public util::Task
{
protected:
    time_t m_start;
    time_t m_end;

public:
    Recording(util::Scheduler *poller, 
	      time_t start, time_t end);

    virtual unsigned int Run() override = 0;
};

typedef util::CountedPointer<Recording> RecordingPtr;

} // namespace tv

#endif
