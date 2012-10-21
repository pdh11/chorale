#ifndef CHORALED_MAIN_H
#define CHORALED_MAIN_H 1

#include "config.h"

#if HAVE_SYS_SYSLOG_H
#include <sys/syslog.h>
#endif

namespace util { class PollerInterface; }

namespace choraled {

/** Values for Settings::flags
 */
enum {
    AUDIO = 1,
    MEDIA_SERVER = 2,
    RECEIVER = 4,
    DVB = 8,
    CD = 0x10,
    LOCAL_DB = 0x20,
    ASSIMILATE_RECEIVER = 0x40
};

struct Settings
{
    unsigned int flags;
    const char *database_file;
    const char *timer_database_file;
    const char *web_root;
    const char *receiver_software_server; // NULL means use ARF
    const char *receiver_arf_file;
    const char *dvb_channels_file;
    const char *media_root;
    const char *flac_root;
    unsigned short web_port;
    unsigned int nthreads;
};

class Complaints
{
public:
    virtual ~Complaints() {}

    virtual void Complain(int loglevel, const char *format, ...) = 0;
};

enum {
#if !HAVE_DECL_LOG_ERR
    LOG_ERR,
#endif
#if !HAVE_DECL_LOG_NOTICE
    LOG_NOTICE,
#endif
#if !HAVE_DECL_LOG_WARNING
    LOG_WARNING,
#endif
    dummy_entry
};

extern volatile bool s_exiting;
extern volatile bool s_rescan;
extern util::PollerInterface *s_poller;

int Main(const Settings*, Complaints*);

#define HAVE_DVB (HAVE_LINUX_DVB_DMX_H && HAVE_LINUX_DVB_FRONTEND_H)

} // namespace choraled

#endif
