/* libutil/trace.h
 *
 * Debug tracing
 */

#ifndef LIBUTIL_TRACE_H
#define LIBUTIL_TRACE_H

#include <stdio.h>
#include <string>
#include <map>
#include <set>
#include <boost/thread/mutex.hpp>

class Tracer
{
    static boost::mutex sm_mutex;
    boost::mutex::scoped_lock m_lock;

public:
    Tracer(const char *file, unsigned int line)
	: m_lock(sm_mutex)
    {
#if 0 //def WIN32
	printf("%04u:%-25s:%4u: ", (unsigned int)GetCurrentThreadId(), file, 
	       line);
#else
	printf("%-25s:%4u: ", file, line);
#endif
    }
    ~Tracer() { fflush(stdout); }

    // scoped_lock is noncopyable in older versions of Boost
    Tracer(const Tracer&) : m_lock(sm_mutex) {}
};

class NullTracer
{
};

inline const Tracer& operator<<(const Tracer& n, const char* s) { printf("%s",s?s:"NULL"); return n; }
inline const Tracer& operator<<(const Tracer& n, const std::string& s) { printf("%s", s.c_str()); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned int ui) { printf("%u",ui); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned long ul) { printf("%lu",ul); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned long long ull) { printf("%llu",ull); return n; }
inline const Tracer& operator<<(const Tracer& n, int i) { printf("%d",i); return n; }
inline const Tracer& operator<<(const Tracer& n, long i) { printf("%ld",i); return n; }
inline const Tracer& operator<<(const Tracer& n, long long i) { printf("%lld",i); return n; }
inline const Tracer& operator<<(const Tracer& n, const void *p) { printf("%p",p); return n; }
inline const Tracer& operator<<(const Tracer& n, double d) { printf("%f",d); return n; }

template<typename X> 
    inline const Tracer& operator<<(const Tracer& n, const std::set<X>& m)
{
    n << "{ ";
    for (typename std::set<X>::const_iterator i = m.begin(); i != m.end(); ++i)
    {
	n << *i << ", ";
    }
    n << "}\n";
    return n;
}

template<typename X, typename Y> 
    inline const Tracer& operator<<(const Tracer& n, const std::map<X,Y>& m)
{
    n << "[ ";
    for (typename std::map<X,Y>::const_iterator i = m.begin(); i != m.end(); ++i)
    {
	n << i->first << "=" << i->second << " ";
    }
    n << "]\n";
    return n;
}

template<typename X, typename Y> 
    inline const Tracer& operator<<(const Tracer& n, const std::pair<X,Y>& p)
{
    n << "(" << p.first << ", " << p.second << ")";
    return n;
}

class Hex
{
    const void *m_address;
    size_t m_nbytes;
public:
    Hex(const void *address, size_t nbytes)
	: m_address(address), m_nbytes(nbytes) {}

    static void Dump(const void *address, size_t nbytes);

    friend const Tracer& operator<<(const Tracer&n, Hex hex);
};

inline const Tracer& operator<<(const Tracer&n, Hex hex)
{
    Hex::Dump(hex.m_address, hex.m_nbytes);
    return n;
}

template <typename T>
inline const NullTracer& operator<<(const NullTracer& n, T) { return n; }

#ifdef WITH_DEBUG
#define TRACE Tracer(__FILE__,__LINE__)
#else
#define TRACE NullTracer()
#endif

#endif
