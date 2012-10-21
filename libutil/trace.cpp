#include "trace.h"
#include "config.h"
#include "utf8.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/tss.hpp>
#include <boost/format.hpp>
#include <stdarg.h>
#include <sys/time.h>
#include <pthread.h>

namespace util {

class ThreadID
{
    static boost::thread_specific_ptr<unsigned int> sm_ptr;
    static boost::mutex sm_mutex;
    static unsigned int sm_next_id;

public:
    static unsigned int Get();
};

unsigned int ThreadID::Get()
{
    unsigned int *ptr = sm_ptr.get();
    if (ptr)
	return *ptr;

    boost::mutex::scoped_lock lock(sm_mutex);
    ptr = sm_ptr.get(); // Double-checked locking pattern
    if (ptr)
	return *ptr;

    unsigned int id = ++sm_next_id;
    sm_ptr.reset(new unsigned int(id));
    return id;
}

boost::thread_specific_ptr<unsigned int> ThreadID::sm_ptr;
boost::mutex ThreadID::sm_mutex;
unsigned int ThreadID::sm_next_id = 0;

boost::mutex Tracer::sm_mutex;

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

#ifdef WITH_DEBUG
LogFileOpener s_logfile_opener;
#endif

Tracer::Tracer(const char *env_var, const char *file, unsigned int line)
    : m_lock(sm_mutex),
      m_emit(false)
{
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
	}
	if (getenv("LOG_TID"))
	{
	    if (s_logfile)
		fprintf(s_logfile, "%03u:", ThreadID::Get());
	    else
		printf("%03u:", ThreadID::Get());
	}

	if (!strncmp(file, "../", 3))
	    file += 3;

	if (s_logfile)
	    fprintf(s_logfile, "%-25s:%4u: ", file, line);
	else
	    printf("%-25s:%4u: ", file, line);
    }
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
	fflush(s_logfile);

    fflush(stdout);
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

const Tracer& operator<<(const Tracer& n, const std::wstring& ws)
{
    return n << ws.c_str();
}

const Tracer& operator<<(const Tracer& n, unsigned long long ull)
{
    /* When compiling with mingw, GCC checks the format argument as if
     * it's Posix (where unsigned long is %llu), but printf and scanf
     * are actually implemented inside the C runtime, where unsigned
     * long is %I64u. Yet GCC warns if you use %I64u. So we tell the
     * pair of them to get knotted, and use Boost instead.
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
