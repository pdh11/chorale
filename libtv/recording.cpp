#include "recording.h"
#include "libutil/counted_pointer.h"
#include "libutil/scheduler.h"
#include "libutil/bind.h"

namespace tv {

Recording::Recording(util::Scheduler *poller,
		     time_t start, time_t end)
    : m_start(start),
      m_end(end)
{
    poller->Wait(
	util::Bind(RecordingPtr(this)).To<&Recording::Run>(),
	start - 5, 0);
}

} // namespace tv
