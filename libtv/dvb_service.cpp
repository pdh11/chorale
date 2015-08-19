#include "config.h"
#include "dvb_service.h"
#include "dvb.h"
#include "dvb_recording.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#include "libutil/printf.h"
#include "libutil/stream.h"

namespace tv {

namespace dvb {

Service::Service(Frontend *frontend, Channels *channels,
		 const std::string &directory,
		 util::Scheduler *poller, util::TaskQueue *disk_queue)
    : m_frontend(frontend),
      m_channels(channels),
      m_recordings_directory(directory),
      m_poller(poller),
      m_disk_queue(disk_queue)
{
}

Service::~Service()
{
}

unsigned int Service::Record(unsigned int channel, time_t start, time_t end,
			const std::string& title)
{
    Channel ch = *(m_channels->begin() + channel);
    struct tm stm;
#if HAVE_LOCALTIME_R
    localtime_r(&start, &stm);
#else
    stm = *localtime(&start);
#endif
    std::string fname = util::SPrintf("%s %04u-%02u-%02u %02u:%02u",
				      title.c_str(),
				      stm.tm_year + 1900,
				      stm.tm_mon + 1,
				      stm.tm_mday,
				      stm.tm_hour,
				      stm.tm_min);

    fname = util::ProtectLeafname(fname);

    if (ch.videopid)
	fname = fname + ".mpg";
    else
	fname = fname + ".mp2";

    fname = m_recordings_directory + "/" + fname;

    TRACE << "Trying to record " << fname << "\n";

    util::MkdirParents(fname.c_str());

    new tv::dvb::Recording(m_poller, m_disk_queue,
			   this, channel,
			   start, end,
			   fname);
    return 0;
}

unsigned int Service::GetStream(unsigned int channel, 
				std::unique_ptr<util::Stream> *pstm)
{
    *pstm = m_frontend->GetStream(*(m_channels->begin()+channel));
    return 0;
}

} // namespace dvb

} // namespace tv
