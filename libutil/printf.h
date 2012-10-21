#ifndef LIBUTIL_PRINTF_H
#define LIBUTIL_PRINTF_H 1

#include <string>
#include <stdio.h>
#include "attributes.h"

namespace util {

// String classes

struct String0
{
    static const char s[];
};

template <char c>
struct String1
{
    static const char s[];
};

template <char c1, char c2>
struct String2
{
    static const char s[];
};

template <char c1, char c2>
const char String2<c1,c2>::s[] = { c1, c2, 0 };

template <char c1, char c2, char c3>
struct String3
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4>
struct String4
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4>
const char String4<c1,c2,c3,c4>::s[] = { c1, c2, c3, c4, 0 };

template <char c1, char c2, char c3, char c4, char c5>
struct String5
{
    static const char s[];
};

template <char,char,char,char,char,char>
struct String6
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4, char c5, char c6>
const char String6<c1,c2,c3,c4,c5,c6>::s[] =
{ c1,c2,c3,c4,c5,c6,0 };

template <char,char,char,char,char,char,char>
struct String7
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7>
const char String7<c1,c2,c3,c4,c5,c6,c7>::s[] =
{ c1,c2,c3,c4,c5,c6,c7,0 };

template <char,char,char,char,char,char,char,char>
struct String8
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7,
	  char c8>
const char String8<c1,c2,c3,c4,c5,c6,c7,c8>::s[] =
{ c1,c2,c3,c4,c5,c6,c7,c8,0 };

template <char,char,char,char,char,char,char,char,char>
struct String9
{
    static const char s[];
};

template <char,char,char,char,char,char,char,char,char,char>
struct String10
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7,
	  char c8, char c9, char c10>
const char String10<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10>::s[] =
{ c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,0 };

template <char,char,char,char,char,char,char,char,char,char,char>
struct String11
{
    static const char s[];
};

template <char,char,char,char,char,char,char,char,char,char,char,char>
struct String12
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7,
	  char c8, char c9, char c10, char c11, char c12>
const char String12<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12>::s[] =
{ c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,0 };

template <char,char,char,char,char,char,char,char,char,char,char,char,char>
struct String13
{
    static const char s[];
};

template <char,char,char,char,char,char,char,char,char,char,char,char,char,char>
struct String14
{
    static const char s[];
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7,
	  char c8, char c9, char c10, char c11, char c12, char c13, char c14>
const char String14<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14>::s[] =
{ c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,0 };

template <class T1, char>
struct Append;

template <char c1>
struct Append<String0, c1>
{
    typedef String1<c1> type;
};

template <char c1, char c2>
struct Append<String1<c1>, c2>
{
    typedef String2<c1,c2> type;
};

template <char c1, char c2, char c3>
struct Append<String2<c1, c2>, c3>
{
    typedef String3<c1,c2,c3> type;
};

template <char c1, char c2, char c3, char c>
struct Append<String3<c1, c2, c3>, c>
{
    typedef String4<c1,c2,c3,c> type;
};

template <char c1, char c2, char c3, char c4, char c>
struct Append<String4<c1, c2, c3,c4>, c>
{
    typedef String5<c1,c2,c3,c4,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c>
struct Append<String5<c1,c2,c3,c4,c5>, c>
{
    typedef String6<c1,c2,c3,c4,c5,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c>
struct Append<String6<c1,c2,c3,c4,c5,c6>, c>
{
    typedef String7<c1,c2,c3,c4,c5,c6,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7, char c>
struct Append<String7<c1,c2,c3,c4,c5,c6,c7>, c>
{
    typedef String8<c1,c2,c3,c4,c5,c6,c7,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7, 
	  char c8, char c>
struct Append<String8<c1,c2,c3,c4,c5,c6,c7,c8>, c>
{
    typedef String9<c1,c2,c3,c4,c5,c6,c7,c8,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7, 
	  char c8, char c9, char c>
struct Append<String9<c1,c2,c3,c4,c5,c6,c7,c8,c9>, c>
{
    typedef String10<c1,c2,c3,c4,c5,c6,c7,c8,c9,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7, 
	  char c8, char c9, char c10, char c>
struct Append<String10<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10>, c>
{
    typedef String11<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7, 
	  char c8, char c9, char c10, char c11, char c>
struct Append<String11<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11>, c>
{
    typedef String12<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7, 
	  char c8, char c9, char c10, char c11, char c12, char c>
struct Append<String12<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12>, c>
{
    typedef String13<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c> type;
};

template <char c1, char c2, char c3, char c4, char c5, char c6, char c7, 
	  char c8, char c9, char c10, char c11, char c12, char c13, char c>
struct Append<String13<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13>, c>
{
    typedef String14<c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c> type;
};

template <class StringN, char c1, char c2>
struct Append2: public Append<typename Append<StringN, c1>::type, c2>
{
};

template <class StringN, char c1, char c2, char c3>
struct Append3: public Append<typename Append<typename Append<StringN, c1>::type, c2>::type, c3>
{
};

template <class StringN, char c1, char c2, char c3, char c4>
struct Append4: public Append<typename Append<typename Append<typename Append<StringN, c1>::type, c2>::type, c3>::type, c4>
{
};

// Printf classes

template <class S, class T>
struct format_traits;

template <class S>
struct format_traits<S, int> : public Append2<S, '%', 'd'> {};

template <class S>
struct format_traits<S, unsigned long long> : public Append4<S, '%', 'l', 'l', 'u'> {};

template <class S>
struct format_traits<S, unsigned long> : public Append3<S, '%', 'l', 'u'> {};

template <class S>
struct format_traits<S, unsigned int> : public Append2<S, '%', 'u'> {};

template <class S>
struct format_traits<S, unsigned short> : public Append2<S, '%', 'u'> {};

template <class S>
struct format_traits<S, const char*> : public Append2<S, '%', 's'> {};

template <class S, class T>
struct format_traits<S, T*> : public Append2<S, '%', 'x'> {};

template <class T>
struct arg_traits
{
    typedef T type;
};

template<>
struct arg_traits<std::string>
{
    typedef const char *type;
};

template<>
template <class T>
struct arg_traits<T*>
{
    typedef const T *type;
};

template <class T>
inline typename arg_traits<T>::type PrintfArg(T t)
{
    return t;
}

template<>
inline const char *PrintfArg(std::string s)
{
    return s.c_str();
}

std::string SPrintf(const char *fmt, ...)
    ATTRIBUTE_PRINTF(1,2);

struct Printf
{
    struct format
    {
	typedef String0 type;
    };

    enum { N = 0 };
};

namespace printf {

template <class T, int n>
struct Output;

template <>
template <class A>
struct Output<A,7>
{
    static std::string String(const A& a)
    {
	return SPrintf(A::format::type::s, a.m.m.m.m.m.m.t,
		       a.m.m.m.m.m.t, a.m.m.m.m.t, 
		       a.m.m.m.t, a.m.m.t, a.m.t, a.t);
    }
};

template <>
template <class A>
struct Output<A,6>
{
    static std::string String(const A& a)
    {
	return SPrintf(A::format::type::s, a.m.m.m.m.m.t, a.m.m.m.m.t, 
		       a.m.m.m.t, a.m.m.t, a.m.t, a.t);
    }
};

template <>
template <class A>
struct Output<A,5>
{
    static std::string String(const A& a)
    {
	return SPrintf(A::format::type::s, a.m.m.m.m.t, a.m.m.m.t, a.m.m.t,
			a.m.t, a.t);
    }
};

template <>
template <class A>
struct Output<A,4>
{
    static std::string String(const A& a)
    {
	return SPrintf(A::format::type::s, a.m.m.m.t, a.m.m.t, a.m.t, a.t);
    }
};

template <>
template <class A>
struct Output<A,3>
{
    static std::string String(const A& a)
    {
	return SPrintf(A::format::type::s, a.m.m.t, a.m.t, a.t);
    }
};

template <>
template <class A>
struct Output<A,2>
{
    static std::string String(const A& a)
    {
	return SPrintf(A::format::type::s, a.m.t, a.t);
    }
};


template <>
template <class A>
struct Output<A,1>
{
    static std::string String(const A& a)
    {
	return SPrintf(A::format::type::s, a.t);
    }
};

template <class MoreArgs, class T>
struct Args
{
    MoreArgs m;
    T t;

    struct format: public format_traits<typename MoreArgs::format::type, T>
    {
    };

    enum { N = MoreArgs::N + 1 };

    operator std::string() const
    {
	return Output<Args<MoreArgs, T>, N >::String(*this);
    }
};

template <class A, class T>
Args<A,typename arg_traits<T>::type> operator<<(const A& a, T t)
{
    Args<A, typename arg_traits<T>::type> a2;
    a2.m = a;
    a2.t = PrintfArg(t);
    return a2;
}

} // namespace printf

template<class T>
printf::Args<Printf,typename arg_traits<T>::type> operator<<(const Printf&, T t)
{
    printf::Args<Printf, typename arg_traits<T>::type> a;
    a.t = PrintfArg(t);
    return a;
}

} // namespace util

#endif
