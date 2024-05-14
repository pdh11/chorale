#include "trace.h"
#include <stdint.h>
#include "config.h"
#include "utf8.h"
#include "mutex.h"
#include <boost/format.hpp>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <map>
#if HAVE_LINUX_UNISTD_H
#include <linux/unistd.h>
#endif

namespace util {

static Mutex s_trace_mutex;

static FILE *s_logfile = NULL;

class LogFileOpener
{
public:
    LogFileOpener();
    ~LogFileOpener();
};

LogFileOpener::LogFileOpener()
{
    const char *filename = getenv("LOG_FILE");

#ifdef __APPLE__
    if (!filename)
	filename = "/var/mobile/Applications/log.txt";
#endif

    if (filename)
    {
	s_logfile = fopen(filename, "wb+");
	if (s_logfile)
	    fprintf(stderr, "*** Logging to %s\n", filename);
	else
	    fprintf(stderr, "Can't open log file '%s': %u\n", filename, errno);
    }
}

LogFileOpener::~LogFileOpener() 
{
    if (s_logfile)
    {
	fclose(s_logfile); 
	s_logfile = NULL;
    }
}

#if DEBUG
LogFileOpener s_logfile_opener;
#endif

Tracer::Tracer(const char *env_var, const char *file, unsigned int line)
    : m_emit(false)
{
    s_trace_mutex.Acquire();

#ifdef __APPLE__
    m_emit = true;
#endif

    if (getenv("LOG_ALL"))
	m_emit = true;
    else
    {
	if (env_var)
	{
	    const char *value = getenv(env_var);
	    if (value && *value)
		m_emit = true;
	}
	else
	    m_emit = true;
    }

    if (m_emit)
    {
	if (getenv("LOG_TIME"))
	{
	    struct timeval tv;
	    gettimeofday(&tv, NULL);

	    if (s_logfile)
		fprintf(s_logfile, "%09u.%06u:", (unsigned)tv.tv_sec,
			(unsigned)tv.tv_usec);
	    else
		printf("%09u.%06u:", (unsigned)tv.tv_sec,
		       (unsigned)tv.tv_usec);

//	    struct timespec tp;
//	    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tp);
//	    printf("%02u.%09u:", (unsigned)tp.tv_sec,
//		   (unsigned)tp.tv_nsec);
	}
	if (getenv("LOG_TID"))
	{
	    /// @todo boost::thread_id
#if HAVE_GETTID
	    unsigned int raw_tid = gettid();
#elif HAVE_NR_GETTID
	    unsigned int raw_tid = (unsigned int)syscall(__NR_gettid);
#elif HAVE_GETCURRENTTHREADID
	    unsigned int raw_tid = ::GetCurrentThreadId();
#else
#warning "No thread id implementation on this platform"
	    unsigned int raw_tid = 0;
#endif
	    static std::map<unsigned int, unsigned int> tidmap;
	    if (tidmap.find(raw_tid) == tidmap.end())
	    {
		// We're still under the Tracer sm_mutex so this is safe
		unsigned int tid = (unsigned int)tidmap.size();
		tidmap[raw_tid] = tid;
	    }

	    if (s_logfile)
		fprintf(s_logfile, "%03u:", tidmap[raw_tid]);
	    else
		printf("%03u:", tidmap[raw_tid]);
	}

	if (!strncmp(file, "../", 3))
	    file += 3;

	if (s_logfile)
	    fprintf(s_logfile, "%-25s:%4u: ", file, line);
	else
	    printf("%-25s:%4u: ", file, line);
    }
}

Tracer::Tracer(const Tracer&)
    : m_emit(false)
{
}

void Tracer::Printf(const char *format, ...) const
{
    if (!m_emit)
	return;

    va_list args;
    va_start(args, format);
    if (s_logfile)
	vfprintf(s_logfile, format, args);
    else
	vprintf(format, args);
    va_end(args);
}

Tracer::~Tracer()
{
    if (s_logfile)
    {
	fflush(s_logfile);
#if HAVE_FSYNC
	fsync(fileno(s_logfile));
#endif
    }

    fflush(stdout);

    s_trace_mutex.Release();
}


LogNameList *LogNameList::sm_head = NULL;

LogNameList::LogNameList(const char *name)
    : m_name(name),
      m_next(NULL)
{
    LogNameList** phead = &sm_head;

    while (*phead)
    {
	if (!strcmp((*phead)->m_name, name))
	    return;
	phead = &(*phead)->m_next;
    }
    *phead = this;
}

void LogNameList::ShowLogNames()
{
    if (sm_head)
    {
	printf("Log by setting these system variables:");
	for (const LogNameList *ptr = sm_head; ptr; ptr = ptr->m_next)
	{
	    printf(" LOG_%s", ptr->m_name);
	}
	printf("\n    or LOG_ALL.\n");
    }
}

void Hex::Dump(const void *address, size_t nbytes)
{
    unsigned int offset = 0;

    while (nbytes)
    {
	unsigned int line = 16;
	if (line > nbytes)
	    line = (unsigned int)nbytes;
	printf("%08x ", offset);
	unsigned int i;
	for (i=0; i<line; ++i)
	    printf(" %02x", ((const unsigned char*)address)[offset+i]);
	for ( ; i<16; ++i)
	    printf("   ");
	printf(" |");
	for (i=0; i<line; ++i)
	{
	    unsigned char c = ((const unsigned char*)address)[offset+i];
	    if (c<32 || c >= 127)
		printf(".");
	    else
		printf("%c", c);
	}
	for ( ; i<16; ++i)
	    printf(" ");
	printf("|\n");

	nbytes -= line;
	offset += line;
    }
}

const Tracer& operator<<(const Tracer& n, const wchar_t* ws)
{
    if (!ws)
	return n << "NULL";

    std::string s = util::WideToUTF8(ws);
    return n << s;
}

const Tracer& operator<<(const Tracer& n, wchar_t* ws)
{
    if (!ws)
	return n << "NULL";

    std::string s = util::WideToUTF8(ws);
    return n << s;
}

const Tracer& operator<<(const Tracer& n, const std::wstring& ws)
{
    return n << ws.c_str();
}

const Tracer& operator<<(const Tracer& n, unsigned long long ull)
{
    /* When compiling with mingw, GCC checks the format argument as if
     * it's Posix (where unsigned long long is %llu), but printf and
     * scanf are actually implemented inside the C runtime, where
     * unsigned long long is %I64u. Yet GCC warns if you use %I64u. So
     * we tell the pair of them to get knotted, and use Boost instead.
     */
    n.Printf("%s", (boost::format("%llu") % ull).str().c_str());
    return n;
}

const Tracer& operator<<(const Tracer& n, long long ll)
{
    n.Printf("%s", (boost::format("%lld") % ll).str().c_str());
    return n;
}

} // namespace util

#ifdef TEST

#include <set>
#include <vector>
#include <map>
#include <list>

struct CaselessCompare
{
    bool operator()(const std::string& s1, const std::string& s2) const
    {
	return strcasecmp(s1.c_str(), s2.c_str()) < 0;
    }
};

int main()
{
    bool b = false;
    char c = 'a';
    signed char sc = -3;
    unsigned char uc = 200;
    short ss = -257;
    unsigned short us = 40000;
    int si = -80000;
    unsigned int ui =        3000000000u;
    long sl = -80001;
    unsigned long ul =       3000000001u;
    int64_t  sll =  INT64_C(-5000000000);
    uint64_t ull = UINT64_C( 5000000000);

    std::string s = "X";
    std::wstring ws = L"Y";

    std::set<int> is;
    std::map<int, int> iim;
    std::vector<int> iv;
    std::list<int> il;

    std::map<int, std::set<int> > iism;
    std::vector<std::map<int, std::set<int> > > iismv;
    std::list<std::vector<std::map<int, std::set<int> > > > iismvl;

    std::map<std::string, std::string, CaselessCompare> sscm;

    TRACE << b << " "
	  << c << " " << sc << " " << uc << " "
	  << ss << " " << us << " "
	  << si << " " << ui << " "
	  << sl << " " << ul << " "
	  << sll << " " << ull << " "
	  << s << " " << ws << " "
	  << s.c_str() << " " << ws.c_str() << " "
	  << is << " " << iim << " " << iv << " "
	  << iism << " " << iismv << " " << iismvl;
    TRACE << sscm;

    return 0;
}

#endif
