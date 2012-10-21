#include "config.h"
#include "dvb.h"
#include "dvb_audio_stream.h"
#include "dvb_video_stream.h"
#include "program_stream.h"
#include "libutil/trace.h"
#include "libutil/printf.h"
#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if HAVE_DVB

#include <sys/ioctl.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

namespace tv {

namespace dvb {

Channels::Channels()
{
}

struct StringMap
{
    const char *s;
    unsigned int val;
};

/* awk < /usr/include/linux/dvb/frontend.h '{ print "{ \""$1"\",", $1 "}," }'
 */
static const StringMap inversions[] = {
    { "INVERSION_AUTO", INVERSION_AUTO },
    { "INVERSION_OFF",  INVERSION_OFF },
    { "INVERSION_ON",   INVERSION_ON },
    { NULL, 0 }
};

static const StringMap bandwidths[] = {
    { "BANDWIDTH_8_MHZ", BANDWIDTH_8_MHZ },
    { "BANDWIDTH_7_MHZ", BANDWIDTH_7_MHZ },
    { "BANDWIDTH_6_MHZ", BANDWIDTH_6_MHZ },
    { "BANDWIDTH_AUTO",	 BANDWIDTH_AUTO },
    { NULL, 0 }
};

static const StringMap fecs[] = {
    { "FEC_NONE", FEC_NONE},
    { "FEC_1_2", FEC_1_2 },
    { "FEC_2_3", FEC_2_3 },
    { "FEC_3_4", FEC_3_4 },
    { "FEC_4_5", FEC_4_5 },
    { "FEC_5_6", FEC_5_6 },
    { "FEC_6_7", FEC_6_7 },
    { "FEC_7_8", FEC_7_8 },
    { "FEC_8_9", FEC_8_9 },
    { "FEC_AUTO", FEC_AUTO },
    { NULL, 0 }
};

static const StringMap qams[] = {
    { "QPSK", QPSK},
    { "QAM_16", QAM_16},
    { "QAM_32", QAM_32},
    { "QAM_64", QAM_64},
    { "QAM_128", QAM_128},
    { "QAM_256", QAM_256},
    { "QAM_AUTO", QAM_AUTO},
    { "VSB_8", VSB_8},
    { "VSB_16", VSB_16},
    { NULL, 0 }
};

static const StringMap modes[] = {
    { "TRANSMISSION_MODE_2K",   TRANSMISSION_MODE_2K },
    { "TRANSMISSION_MODE_8K",   TRANSMISSION_MODE_8K },
    { "TRANSMISSION_MODE_AUTO", TRANSMISSION_MODE_AUTO },
    { NULL, 0 }
};

static const StringMap guards[] = {
    { "GUARD_INTERVAL_1_32", GUARD_INTERVAL_1_32 },
    { "GUARD_INTERVAL_1_16", GUARD_INTERVAL_1_16 },
    { "GUARD_INTERVAL_1_8", GUARD_INTERVAL_1_8 },
    { "GUARD_INTERVAL_1_4", GUARD_INTERVAL_1_4 },
    { "GUARD_INTERVAL_AUTO", GUARD_INTERVAL_AUTO },
    { NULL, 0 }
};

static const StringMap hierarchies[] = {
    { "HIERARCHY_NONE", HIERARCHY_NONE},
    { "HIERARCHY_1", HIERARCHY_1},
    { "HIERARCHY_2", HIERARCHY_2},
    { "HIERARCHY_4", HIERARCHY_4},
    { "HIERARCHY_AUTO", HIERARCHY_AUTO},
    { NULL, 0 }
};

static unsigned int WhichString(const char *s, const StringMap *table)
{
    while (table->s)
    {
	if (!strcmp(table->s, s))
	    return table->val;
	++table;
    }
    return 0;
}

unsigned int Channels::Load(const char *conffile)
{
    FILE *f = fopen(conffile, "r");
    if (!f)
    {
	TRACE << "Can't open DVB config file " << conffile << "\n";
	return (unsigned)errno;
    }

    char buffer[200];

    /** Example line:

BBC 6 Music:842000000:INVERSION_AUTO:BANDWIDTH_8_MHZ:FEC_3_4:FEC_3_4:QAM_16:TRANSMISSION_MODE_2K:GUARD_INTERVAL_1_32:HIERARCHY_NONE:0:432:18048

    * (13 fields)
    */

    while (fgets(buffer, sizeof(buffer), f))
    {
	char *ptrs[13];
	char *saveptr = NULL;
	ptrs[0] = strtok_r(buffer, ":", &saveptr);
	for (unsigned int i=1; i<13; ++i)
	    ptrs[i] = strtok_r(NULL, ":", &saveptr);

	if (ptrs[12])
	{
	    Channel ch;
	    ch.name = ptrs[0];
	    ch.frequency = (unsigned)strtoul(ptrs[1], NULL, 10);
	    ch.inversion = WhichString(ptrs[2], inversions);
	    ch.bandwidth = WhichString(ptrs[3], bandwidths);
	    ch.coderatehp = WhichString(ptrs[4], fecs);
	    ch.coderatelp = WhichString(ptrs[5], fecs);
	    ch.constellation = WhichString(ptrs[6], qams);
	    ch.transmissionmode = WhichString(ptrs[7], modes);
	    ch.guardinterval = WhichString(ptrs[8], guards);
	    ch.hierarchy = WhichString(ptrs[9], hierarchies);
	    ch.videopid = (unsigned)strtoul(ptrs[10], NULL, 10);
	    ch.audiopid = (unsigned)strtoul(ptrs[11], NULL, 10);
	    ch.service_id = (unsigned)strtoul(ptrs[12], NULL, 10);

	    m_vec.push_back(ch);
	}
	else
	{
	    TRACE << "Don't like line " << buffer << "\n";
	}
    }
    fclose(f);

    return 0;
}

Frontend::Frontend()
    : m_fd(-1), m_hz(0)
{
}

Frontend::~Frontend()
{
    if (m_fd >= 0)
    {
//	TRACE << "Closing frontend\n";
	::close(m_fd);
    }
}

unsigned int Frontend::Open(unsigned int adaptor, unsigned int device)
{
    std::string devnode = util::Printf() << "/dev/dvb/adapter"
					 << adaptor << "/frontend" << device;
    m_fd = ::open(devnode.c_str(), O_RDWR);
//    TRACE << "open(\"" << devnode << "\")=" << m_fd << "\n";
    if (m_fd < 0)
    {
//	TRACE << "errno=" << errno << "\n";
	return (unsigned)errno;
    }
    return 0;
}

unsigned int Frontend::Tune(const Channel& channel)
{
    if (m_hz == channel.frequency)
	return 0;

    struct dvb_frontend_info fei;

    if (::ioctl(m_fd, FE_GET_INFO, &fei) < 0)
    {
	TRACE << "Frontend FE_GET_INFO failed\n";
	return (unsigned)errno;
    }

    if (fei.type != FE_OFDM)
    {
	TRACE << "Frontend device isn't OFDM (DVB-T)\n";
	return EINVAL;
    }

    struct dvb_frontend_parameters fep;
    memset(&fep, 0, sizeof(fep));

    fep.frequency = channel.frequency;
    fep.inversion = (fe_spectral_inversion_t)channel.inversion;
    fep.u.ofdm.bandwidth = (fe_bandwidth_t)channel.bandwidth;
    fep.u.ofdm.code_rate_HP = (fe_code_rate_t)channel.coderatehp;
    fep.u.ofdm.code_rate_LP = (fe_code_rate_t)channel.coderatelp;
    fep.u.ofdm.constellation = (fe_modulation_t)channel.constellation;
    fep.u.ofdm.transmission_mode = (fe_transmit_mode_t)channel.transmissionmode;
    fep.u.ofdm.guard_interval = (fe_guard_interval_t)channel.guardinterval;
    fep.u.ofdm.hierarchy_information = (fe_hierarchy_t)channel.hierarchy;
    
    if (::ioctl(m_fd, FE_SET_FRONTEND, &fep) < 0)
    {
	TRACE << "Frontend FE_SET_FRONTEND failed (" << errno << ")\n";
	return (unsigned)errno;
    }

    TRACE << "Frontend tuned to " << channel.frequency << "Hz\n";

    m_hz = channel.frequency;

    return 0;
}

void Frontend::GetState(unsigned int*, unsigned int*, bool *ptuned)
{
    fe_status_t status;
    ::ioctl(m_fd, FE_READ_STATUS, &status);
    *ptuned = (status & FE_HAS_LOCK) != 0;
    if (*ptuned)
	TRACE << "tuned!\n";
}

std::auto_ptr<util::Stream> Frontend::GetStream(const Channel& c)
{
    std::auto_ptr<util::Stream> sp;

    Tune(c);

    unsigned int tries = 0;
    bool tuned;
    do {
	GetState(NULL, NULL, &tuned);
	if (tuned)
	    break;
	sleep(1);
	++tries;
    } while (tries < 30);

    if (!tuned)
    {
	TRACE << "Can't tune\n";
	return sp;
    }
    
    if (c.videopid)
    {
	VideoStream *vs = new VideoStream;
	if (vs->Open(0u, 0u, c.audiopid, c.videopid) != 0)
	{
	    delete vs;
	    return sp;
	}
	sp.reset(vs);
	return std::auto_ptr<util::Stream>(new ProgramStream(sp));
    }
    else
    {
	AudioStream *as = new AudioStream;
	if (as->Open(0, 0, c.audiopid) != 0)
	{
	    delete as;
	    return sp;
	}
	sp.reset(as);
	return sp;
    }
}

} // namespace dvb

} // namespace tv

#endif // HAVE_DVB


        /* Unit test */


#ifdef TEST

# include "libutil/file_stream.h"
# include "libutil/worker_thread_pool.h"
# include "libutil/async_write_buffer.h"

int main()
{
#if HAVE_DVB
    tv::dvb::Channels c;
    c.Load("/etc/channels.conf");

    tv::dvb::Frontend f;
    unsigned int res = f.Open(0,0);
    if (res != 0)
    {
	TRACE << "Can't open DVB frontend (no hardware?)\n";
	return 0; // Probably no DVB hardware
    }

    if (c.Count() == 0)
    {
	TRACE << "No channels\n";
	return 0;
    }

    f.Tune(*c.begin());

    int tries = 0;
    bool tuned;
    do {
	f.GetState(NULL, NULL, &tuned);
	if (tuned)
	    break;
	sleep(1);
	++tries;
    } while (tries < 5);

    if (!tuned)
    {
	TRACE << "Can't tune\n";
	return 0;
    }

    return 0;
#else
    TRACE << "No DVB available\n";
#endif

    return 0;
}

#endif
