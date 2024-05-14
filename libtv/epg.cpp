#include "config.h"
#include "epg.h"
#include "epg_database.h"
#include "dvb.h"
#include "libdbsteam/db.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libutil/task.h"
#include "libutil/scheduler.h"
#include "libutil/bind.h"
#include "libutil/counted_pointer.h"
#include <set>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#if HAVE_DVB

#include <sys/ioctl.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

namespace tv {

namespace dvb {

class EPGTimerTask;

class EPGParserTask: public util::Task
{
    EPGTimerTask *m_parent;
    util::Scheduler *m_scheduler;
    std::set<unsigned int> m_seen;
    db::Database *m_db;
    time_t m_last_new;
    unsigned int m_count;

    /** Map from service ids to channel indexes */
    typedef std::map<unsigned int, unsigned int> chanmap_t;
    chanmap_t m_chanmap;

    int m_demux_fd;

public:
    EPGParserTask(EPGTimerTask *parent, util::Scheduler *scheduler,
		  db::Database *db, Channels *ch, int demux_fd);
    ~EPGParserTask();

    unsigned int Run() override;
};

typedef util::CountedPointer<EPGParserTask> EPGParserTaskPtr;

class EPGTimerTask: public util::Task
{
    Frontend *m_fe;
    Channels *m_channels;
    util::Scheduler *m_poller;
    epg::Database m_db;
    util::TaskPtr m_parser_task;
    bool m_first_time;

public:
    EPGTimerTask(Frontend*, Channels*, util::Scheduler*, 
		 const char *db_filename, Service *service);

    unsigned int Run() override;
    unsigned int OnParserDone();

    db::Database *GetDatabase() { return &m_db; }
};

typedef util::CountedPointer<EPGTimerTask> EPGTimerTaskPtr;

EPGParserTask::EPGParserTask(EPGTimerTask *parent, util::Scheduler *scheduler,
			     db::Database *db, 
			     Channels *ch, int demux_fd)
    : m_parent(parent),
      m_scheduler(scheduler),
      m_db(db),
      m_last_new(0),
      m_count(0),
      m_demux_fd(demux_fd)
{
    TRACE << "Parser task starting\n";

    unsigned int k=0;
    for (Channels::const_iterator i = ch->begin(); i != ch->end(); ++i, ++k)
    {
//	TRACE << "Listening for service_id " << i->service_id << "\n";
	m_chanmap[i->service_id] = k;
    }

    m_scheduler->WaitForReadable(
	util::Bind(EPGParserTaskPtr(this)).To<&EPGParserTask::Run>(),
	m_demux_fd, false);
}

EPGParserTask::~EPGParserTask()
{
    if (m_demux_fd >= 0)
	::close(m_demux_fd);
}

/* eit table:
 *
 *  0 tableid
 *  1 ssi/lenhi
 *  2 lenlo
 *  3 sidhi
 *  4 sidlo
 *  5 version/cni
 *  6 section
 *  7 last_section
 *  8 tsid_hi
 *  9 tsid_lo
 * 10 netid_hi
 * 11 netid_lo
 * 12 last_section
 * 13 last_tableid
 *
 * eit event:
 *  0 eidhi
 *  1 eidlo
 *  2 datehi
 *  3 datelo (Modified Julian Date, GMT)
 *  4 timeh (all BCD; GMT)
 *  5 timem
 *  6 times
 *  7 durationh (all BCD)
 *  8 durationm
 *  9 durations
 * 10 flags/dllhi
 * 11 dlllo
 */

static void mjdtogreg(unsigned int mjd, int *pyear, int *pmonth, int *pday)
{
    // Wikipedia "Julian Day"
    int J = (int)mjd + 2400001;
    int j = J + 32044;
    int g = j / 146097;
    int dg = j % 146097;
    int c = ((dg/36524 + 1)*3) / 4;
    int dc = dg - c * 36524;
    int b = dc / 1461;
    int db = dc % 1461;
    int a = ((db/365 + 1) * 3) / 4;
    int da = db - a*365;
    int y = g*400 + c*100 + b*4 + a;
    int m = (da*5 + 308)/153 - 2;
    int d = da - ((m+4)*153)/5 + 122;
    *pyear = y - 4800 + (m+2)/12;
    *pmonth = ((m+2)%12) + 1;
    *pday = d+1;
}

static inline unsigned int BCD(unsigned char c) { return (unsigned)(c>>4)*10 + (c&15u); }

unsigned int EPGParserTask::Run()
{
    TRACE << "EPGParser readable\n";

    enum { BUFSIZE = 4096 };
    unsigned char buf[BUFSIZE];

    time_t now = time(NULL);

    for (;;)
    {
	ssize_t sz = ::read(m_demux_fd, buf, BUFSIZE);
	if (sz <= 0)
	{
	    if (errno == EAGAIN ||
                (EAGAIN != EWOULDBLOCK && errno == EWOULDBLOCK))
		break;

	    TRACE << "EPG read got error " << errno << "\n";
	    m_parent->OnParserDone();
	    return (unsigned)errno;
	}

	if (sz < 4)
	    return 0;

	// @todo check crc
	sz -= 4;

	const unsigned char *table = buf;

	unsigned char table_id = *buf;

	if (table_id < 0x4E || table_id > 0x6F)
	{
	    TRACE << "Non-EIT table id " << (unsigned int)table_id << "\n";
	    return 0;
	}

	unsigned int channel = table[3]*256u + table[4];
//    unsigned int tsid = table[8]*256 + table[9];
//    unsigned int netid = table[10]*256 + table[11];

//	TRACE << "EIT table header:\n" << Hex(buf, 14);

	chanmap_t::const_iterator i = m_chanmap.find(channel);
	if (i == m_chanmap.end())
	{
	    // Don't care about this channel
	    return 0;
	}
	unsigned int channel_index = i->second;

	const unsigned char *event = buf+14;

	while (event + 12 < buf + sz)
	{
	    unsigned int eventid = event[0]*256u + event[1];
	    unsigned int mjd = event[2]*256u + event[3];

	    int y, m, d;
	    mjdtogreg(mjd, &y, &m, &d);

	    unsigned int h = BCD(event[4]);
	    unsigned int min = BCD(event[5]);
	    unsigned int s = BCD(event[6]);

	    unsigned int dh = BCD(event[7]);
	    unsigned int dm = BCD(event[8]);
	    unsigned int ds = BCD(event[9]);

	    unsigned int desclen = (event[10] & 0xFu)*256u + event[11];
	    std::string title;
	    std::string desc;

	    unsigned int offset = 0;

	    while (offset < desclen)
	    {
		unsigned int tag = event[12+offset];
		unsigned int len = event[12+offset+1];

		switch (tag)
		{
		case 0x4D:
		    /* <lang><titlelen>title<desclen>desc */
		    if (len > 3)
		    {
			const unsigned char *tptr = event+12+offset+2+3;
			unsigned int tlen = *tptr++;
			if (tlen + 4 <= len)
			{
			    title = std::string((const char*)tptr,
                                                (const char*)tptr+tlen);
			    tptr += tlen;
			    unsigned int dlen = *tptr++;
			    if (tlen+dlen+5 <= len)
			    {
				desc = std::string((const char*)tptr,
                                                   (const char*)tptr+dlen);
			    }
			}
		    }
		    break;

		case 0x50: // Format and language
		case 0x54: // Content type
		case 0x76: // CRID
		case 0x53: // CA data
		    break;

		default:
		    TRACE << "Unknown tag " << tag << "(len " << len << ")\n";
		    break;
		}

		offset += len + 2;
	    }

	    unsigned int token = (channel << 16) + eventid;
	    if (m_seen.find(token) == m_seen.end())
	    {
		m_last_new = now;
		db::QueryPtr qp = m_db->CreateQuery();
		qp->Where(qp->Restrict(tv::epg::ID, db::EQ, token));
		db::RecordsetPtr rs = qp->Execute();
		if (!rs || rs->IsEOF())
		{
		    rs = m_db->CreateRecordset();
		    rs->AddRecord();
		}
		unsigned int start_timet =
		    (mjd-40587)*86400 + h*3600 + min*60 + s;
		unsigned int duration_s = (dh*3600 + dm*60 + ds);
		unsigned int end_timet = start_timet + duration_s;
		rs->SetInteger(tv::epg::ID, token);
		rs->SetString(tv::epg::TITLE, title);
		rs->SetString(tv::epg::DESCRIPTION, desc);
		rs->SetInteger(tv::epg::START, start_timet);
		rs->SetInteger(tv::epg::END, end_timet);
		rs->SetInteger(tv::epg::CHANNEL, channel_index);
		rs->Commit();
		
		m_seen.insert(token);
		++m_count;
		if ((m_count % 1000) == 0)
		{
		    TRACE << "Seen " << m_count << " events\n";
		}
	    }
	    
	    event += 12 + desclen;
	}
    }

    if ((now - m_last_new) > 10)
    {
	TRACE << "Nothing new for ages, quitting\n"; // The SGTL effect
	TRACE << m_count << " events\n";
	/** @todo Clear out expired events
	 */
	m_parent->OnParserDone();
	return 0;
    }
    return 0;
}

EPGTimerTask::EPGTimerTask(Frontend *fe, Channels *ch, util::Scheduler *poller,
			   const char *db_filename, Service *service)
    : m_fe(fe),
      m_channels(ch),
      m_poller(poller),
      m_db(db_filename, service),
      m_first_time(true)
{
    m_poller->Wait(
	util::Bind(EPGTimerTaskPtr(this)).To<&EPGTimerTask::Run>(), 0, 0);
}

unsigned int EPGTimerTask::Run()
{
    TRACE << "Scanning EPG, t=" << time(NULL) << "\n";

    if (m_parser_task)
    {
	TRACE << "Still scanning from last time\n";
	return 0;
    }

    m_fe->Tune(*m_channels->begin());

    unsigned int tries = 0;
    bool tuned;
    do {
	m_fe->GetState(NULL, NULL, &tuned);
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

    int demux_fd = ::open("/dev/dvb/adapter0/demux0", O_RDWR|O_NONBLOCK);
    if (demux_fd < 0)
    {
	TRACE << "Can't open demux device: " << errno << "\n";
	return (unsigned)errno;
    }

    /* Set a big buffer */
    int rc = ::ioctl(demux_fd, DMX_SET_BUFFER_SIZE, 256*1024);
    if (rc < 0)
    {
	TRACE << "Can't set PES buffer size: " << errno << "\n";
	// Not a fatal error
    }

    struct dmx_sct_filter_params sfp;
    memset(&sfp, '\0', sizeof(sfp));

    sfp.pid = 0x12; // EIT
    sfp.timeout = 0;
    sfp.flags = DMX_IMMEDIATE_START;

    rc = ::ioctl(demux_fd, DMX_SET_FILTER, &sfp);
    if (rc < 0)
    {
	TRACE << "Can't set filter: " << errno << "\n";
	return (unsigned)errno;
    }

    TRACE << "Creating parser task\n";

    m_parser_task = util::TaskPtr(new EPGParserTask(this, m_poller, &m_db,
						    m_channels, demux_fd));

    if (m_first_time)
    {
	m_first_time = false;
	time_t epgtime = time(NULL) - 600;
	epgtime -= (epgtime % 43200); // Round to 12-hour interval
	epgtime += 600; // 10 mins after midnight

	m_poller->Wait(
	    util::Bind(EPGTimerTaskPtr(this)).To<&EPGTimerTask::Run>(),
	    epgtime, 12*3600*1000); // Poll every 12h
    }
    return 0;
}

unsigned int EPGTimerTask::OnParserDone()
{
    m_poller->Remove(m_parser_task);
    m_parser_task.reset(NULL);
    TRACE << "All done, t=" << time(NULL) << "\n";

    return 0;
}

} // namespace dvb

} // namespace tv

namespace tv {

namespace dvb {

EPG::EPG()
    : m_db(NULL)
{
}

EPG::~EPG()
{
}

unsigned int EPG::Init(Frontend *fe, Channels *ch, util::Scheduler *poller, 
		       const char *db_filename, Service *service)
{
    if (m_db)
	return EALREADY;

    EPGTimerTask *task = new EPGTimerTask(fe, ch, poller, db_filename,
					  service);
    m_db = task->GetDatabase();
    return 0;
}


} // namespace dvb

} // namespace tv

#endif // HAVE_DVB



        /* Unit test */


#ifdef TEST

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

    util::BackgroundScheduler poller;

    tv::dvb::EPG epg;
    res = epg.Init(&f, &c, &poller, "test-timerdb.xml", NULL);
    assert(res == 0);

    const time_t start = time(NULL);

    while ((time(NULL) - start) < 80)
    {
	poller.Poll(1000);
    }

#else
    TRACE << "No DVB available\n";
#endif // HAVE_DVB

    return 0;
}

#endif // TEST
