/* libutil/trace.h
 *
 * Debug tracing
 */

#ifndef LIBUTIL_TRACE_H
#define LIBUTIL_TRACE_H

#include <string>
#include <memory>

namespace util {

class Tracer
{
    bool m_emit;
public:
    Tracer(const char *env_var, const char *file, unsigned int line);
    ~Tracer();

    void Printf(const char *format, ...) const;

    Tracer(const Tracer& other);
};

inline const Tracer& operator<<(const Tracer& n, const char* s) { n.Printf("%s",s?s:"NULL"); return n; }
inline const Tracer& operator<<(const Tracer& n, char* s) { n.Printf("%s",s?s:"NULL"); return n; }
       const Tracer& operator<<(const Tracer& n, const wchar_t* s);
       const Tracer& operator<<(const Tracer& n, wchar_t* s);
inline const Tracer& operator<<(const Tracer& n, const std::string& s) { n.Printf("%s", s.c_str()); return n; }
       const Tracer& operator<<(const Tracer& n, const std::wstring& s);
inline const Tracer& operator<<(const Tracer& n, bool b) { n.Printf("%s", b?"true":"false"); return n; }
inline const Tracer& operator<<(const Tracer& n, char i) { n.Printf("'%c'",i); return n; }
inline const Tracer& operator<<(const Tracer& n, signed char i) { n.Printf("%d",i); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned char ui) { n.Printf("%u",ui); return n; }
inline const Tracer& operator<<(const Tracer& n, short i) { n.Printf("%d",i); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned short ui) { n.Printf("%u",ui); return n; }
inline const Tracer& operator<<(const Tracer& n, int i) { n.Printf("%d",i); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned int ui) { n.Printf("%u",ui); return n; }
inline const Tracer& operator<<(const Tracer& n, long i) { n.Printf("%ld",i); return n; }
inline const Tracer& operator<<(const Tracer& n, unsigned long ul) { n.Printf("%lu",ul); return n; }
inline const Tracer& operator<<(const Tracer& n, const void *p) { n.Printf("%p",p); return n; }
inline const Tracer& operator<<(const Tracer& n, double d) { n.Printf("%f",d); return n; }
// These ones are out-of-line due to mingw shenanigans
       const Tracer& operator<<(const Tracer& n, unsigned long long ull);
       const Tracer& operator<<(const Tracer& n, long long ll);

template<typename T>
inline const Tracer& operator<<(const Tracer& n, const T* ptr)
{
    return n << ((const void*)ptr);
}

template<typename T>
inline const Tracer& operator<<(const Tracer& n, T* ptr)
{
    return n << ((const void*)ptr);
}

/** For STL sequences (two-parameter) eg vector
 */
template<typename X, typename Y, template<typename,typename> class SEQ> 
inline const Tracer& operator<<(const Tracer& n,
				const SEQ<X,Y>& m)
{
    n << "{ ";
    for (typename SEQ<X,Y>::const_iterator i = m.begin(); i != m.end(); ++i)
    {
	n << *i << ", ";
    }
    n << "}\n";
    return n;
}

/** For STL sequences (three-parameter) eg set
 */
template<typename X, typename Y, typename Z,
	 template<typename,typename,typename> class SEQ> 
inline const Tracer& operator<<(const Tracer& n, const SEQ<X,Y,Z>& m)
{
    n << "{ ";
    for (typename SEQ<X,Y,Z>::const_iterator i = m.begin(); i != m.end(); ++i)
    {
	n << *i << ", ";
    }
    n << "}\n";
    return n;
}

/** For STL sequences (four-parameter) eg map
 */
template<typename W, typename X, typename Y, typename Z,
	 template<typename,typename,typename,typename> class SEQ>
inline const Tracer& operator<<(const Tracer& n, const SEQ<W,X,Y,Z>& m)
{
    n << "{ ";
    for (typename SEQ<W,X,Y,Z>::const_iterator i = m.begin(); i != m.end(); ++i)
    {
	n << *i << ", ";
    }
    n << "}\n";
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

template<typename X>
inline const Tracer& operator<<(const Tracer&n, const std::unique_ptr<X>& ptr)
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
 */
struct NullTracer {
};

template<typename T>
inline const NullTracer& operator<<(const NullTracer& n, const T&) { return n; }

inline const NullTracer& operator<<(const NullTracer& n, int) { return n; }

} // namespace util

#if DEBUG
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
