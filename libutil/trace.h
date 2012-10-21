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
#include <vector>
#include <boost/thread/mutex.hpp>
#include "attributes.h"

namespace util {

class Tracer
{
    static boost::mutex sm_mutex;
    boost::mutex::scoped_lock m_lock;
    bool m_emit;

public:
    Tracer(const char *env_var, const char *file, unsigned int line);
    ~Tracer();

    void Printf(const char *format, ...) const ATTRIBUTE_PRINTF(2,3);

    // scoped_lock is noncopyable in older versions of Boost
    Tracer(const Tracer& o) : m_lock(sm_mutex), m_emit(o.m_emit) {}
};

inline const Tracer& operator<<(const Tracer& n, const char* s) { n.Printf("%s",s?s:"NULL"); return n; }
       const Tracer& operator<<(const Tracer& n, const wchar_t* s);
inline const Tracer& operator<<(const Tracer& n, const std::string& s) { n.Printf("%s", s.c_str()); return n; }
       const Tracer& operator<<(const Tracer& n, const std::wstring& s);
inline const Tracer& operator<<(const Tracer& n, unsigned int ui) { n.Printf("%u",ui); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned long ul) { n.Printf("%lu",ul); return n; }
inline const Tracer& operator<<(const Tracer& n, char i) { n.Printf("'%c'",i); return n; }
inline const Tracer& operator<<(const Tracer& n, int i) { n.Printf("%d",i); return n; }
inline const Tracer& operator<<(const Tracer& n, long i) { n.Printf("%ld",i); return n; }
inline const Tracer& operator<<(const Tracer& n, const void *p) { n.Printf("%p",p); return n; }
inline const Tracer& operator<<(const Tracer& n, double d) { n.Printf("%f",d); return n; }
// These ones are out-of-line due to mingw shenanigans
const Tracer& operator<<(const Tracer& n, unsigned long long ull);
const Tracer& operator<<(const Tracer& n, long long ll);

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

template<typename X> 
inline const Tracer& operator<<(const Tracer& n, const std::vector<X>& v)
{
    n << "( ";
    for (typename std::vector<X>::const_iterator i = v.begin(); i != v.end(); ++i)
    {
	n << *i << ", ";
    }
    n << ")\n";
    return n;
}

template<typename X, typename Y, typename Z> 
inline const Tracer& operator<<(const Tracer& n, const std::map<X,Y,Z>& m)
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

template<typename X, template<typename Y> class SmartPtr>
inline const Tracer& operator<<(const Tracer&n, const SmartPtr<X>& ptr)
{
    n << ptr.get();
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

class LogNameList
{
    const char *m_name;
    static LogNameList *sm_head;
    LogNameList *m_next;

public:
    LogNameList(const char *name);
    static void ShowLogNames();
};

/** A tracer that does nothing, for release builds.
 *
 * For EnumHelper, see
 * http://pdh11.blogspot.com/2009/04/one-template-to-rule-them-all-revisited.html
 */
struct NullTracer {
    NullTracer() {}

    struct EnumHelper {
	EnumHelper() {}
	EnumHelper(const NullTracer&) {}
    } helper;

    NullTracer(const EnumHelper&) {}
};

template<typename T>
inline const NullTracer& operator<<(const NullTracer& n, T) { return n; }

inline const NullTracer::EnumHelper& operator<<(const NullTracer& n, int)
{ return n.helper; }

} // namespace util

#ifdef WITH_DEBUG
#define TRACE ::util::Tracer(NULL, __FILE__, __LINE__)
#define LOG(x) ::util::Tracer(::LOG_IMPL_ ##x, __FILE__, __LINE__)
#define LOG_DECL(x) \
    static util::LogNameList LOG_NAME_ ##x (#x);	\
    static const char LOG_IMPL_ ##x [] = "LOG_" #x
#else
#define TRACE util::NullTracer()
#define LOG(x) util::NullTracer()
#define LOG_DECL(x) extern char LOG_IMPL_ ##x
#endif

#endif
