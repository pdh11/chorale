#ifndef LIBUTIL_XML_H
#define LIBUTIL_XML_H 1

#include <string>
#include <list>
#include <stdlib.h>
#include "stream.h"
#include "counted_pointer.h"

/** Classes for XML support
 *
 * There are two choices of parsing schemes here: SAX-like, where you
 * get told about each tag and attribute as it goes past, and
 * table-driven, where you can get the parser itself to build you a
 * data structure corresponding to the XML. The latter is implemented
 * using a DSEL-like collection of templates: see @ref xml.
 */
namespace xml {

class SaxParserObserver
{
public:
    virtual ~SaxParserObserver() {}
    virtual unsigned int OnBegin(const char* /*tag*/) { return 0; }
    virtual unsigned int OnEnd(const char* /*tag*/) { return 0; }
    virtual unsigned int OnAttribute(const char* /*name*/,
				     const char* /*value*/) { return 0; }
    virtual unsigned int OnContent(const char* /*content*/) { return 0; }
};

/** Simple, SAX-like XML parser.
 *
 * This parser is fairly forgiving of broken XML, which is necessary
 * for parsing Windows ASX playlist files -- they look a bit like XML
 * but aren't really.
 */
class SaxParser
{
    SaxParserObserver *m_observer;

    enum { BUFSIZE = 1024 };

    char m_buffer[BUFSIZE+1];
    size_t m_buffered;
    bool m_eof;
    std::string m_tagname;
    std::string m_attrname;
    
    enum { 
	CONTENT,
	TAG,
	IN_TAG,
	IN_ATTR,
	SEEK_GT,
	ATTR_SEEKEQ,
	ATTR_SEEKVALUE,
	IN_QUOTEDVALUE, 
	IN_VALUE 
    } m_state;

    void Parse();

public:
    explicit SaxParser(SaxParserObserver *observer);

    unsigned int Parse(util::StreamPtr);
    unsigned int WriteAll(const void *buffer, size_t len);
};

class NullSelector;

namespace internals {

struct Data;

unsigned int Parse(util::StreamPtr, void *target, const Data *table);

}

template <typename T1, typename T2>
class AssertSame;

template <typename T>
class AssertSame<T, T> {};

template <class Selector, class Target>
class AssertCorrectTargetType
    : public AssertSame<Target, typename Selector::Target> {};

template <class Target>
class AssertCorrectTargetType<NullSelector, Target> {};


        /* Parser */


/** Base (root) element of a table-driven XML parser; see @ref xml.
 *
 * Makes no callbacks, but constrains its contained elements to only match
 * root elements.
 *
 * @param Selector0 Contained tags
 * @param Selector1 ...
 */
template <class Selector0,
	  class Selector1 = NullSelector,
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class Parser
{
public:
    typedef typename Selector0::Target Target;

    typedef AssertCorrectTargetType<Selector1, Target> assert1;
    typedef AssertCorrectTargetType<Selector2, Target> assert2;
    typedef AssertCorrectTargetType<Selector3, Target> assert3;
    typedef AssertCorrectTargetType<Selector4, Target> assert4;
    typedef AssertCorrectTargetType<Selector5, Target> assert5;
    typedef AssertCorrectTargetType<Selector6, Target> assert6;
    typedef AssertCorrectTargetType<Selector7, Target> assert7;

    static const internals::Data data;

    unsigned int Parse(util::StreamPtr s, Target *obs)
    {
	return internals::Parse(s, (void*)obs, &data);
    }
};


        /* Tag */


/** Match an XML element; see @ref xml.
 *
 * Makes no callbacks, but constrains its contained elements to only match
 * if inside this element.
 *
 * @param name      XML name of the element
 * @param Selector0 Contained tags
 * @param Selector1 ...
 */
template <const char *name,
	  class Selector0,
	  class Selector1 = NullSelector,
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class Tag
{
public:
    typedef typename Selector0::Target Target;

    AssertCorrectTargetType<Selector1, Target> assert1;
    AssertCorrectTargetType<Selector2, Target> assert2;
    AssertCorrectTargetType<Selector3, Target> assert3;
    AssertCorrectTargetType<Selector4, Target> assert4;
    AssertCorrectTargetType<Selector5, Target> assert5;
    AssertCorrectTargetType<Selector6, Target> assert6;
    AssertCorrectTargetType<Selector7, Target> assert7;

    static const internals::Data data;
};


        /* Tag with contents (callback) */


/** Parse out the contents of an XML element; see @ref xml.
 *
 * Makes a callback into the target object, with the contents of an XML
 * element. @sa TagMember.
 *
 * @param name      XML name of the element
 * @param T         C++ type of the target object
 * @param fn       Pointer-to-member of the T object's callback method
 * @param Selector0 Contained tags, if any.
 * @param Selector1 ...
 */
template <const char *name,
	  class T, unsigned int (T::*fn)(const std::string&),
	  class Selector0 = NullSelector, 
	  class Selector1 = NullSelector,
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class TagCallback
{
    static unsigned int OnText(void *obs, const std::string& s)
    {
	return (((T*)obs)->*fn)(s);
    }
public:
    typedef T Target;

    AssertCorrectTargetType<Selector0, Target> assert0;
    AssertCorrectTargetType<Selector1, Target> assert1;
    AssertCorrectTargetType<Selector2, Target> assert2;
    AssertCorrectTargetType<Selector3, Target> assert3;
    AssertCorrectTargetType<Selector4, Target> assert4;
    AssertCorrectTargetType<Selector5, Target> assert5;
    AssertCorrectTargetType<Selector6, Target> assert6;
    AssertCorrectTargetType<Selector7, Target> assert7;

    static const internals::Data data;
};


        /* Tag with contents (member) */


/** Parse out the contents of an XML element; see @ref xml.
 *
 * Stores the contents of an XML element, into a member variable of
 * the target object. @sa TagCallback.
 *
 * @param name      XML name of the element
 * @param T         C++ type of the target object
 * @param pmem      Pointer-to-member of the T object's string member
 * @param Selector0 Contained tags, if any.
 * @param Selector1 ...
 */
template <const char *name,
	  class T, std::string (T::*pmem),
	  class Selector0 = NullSelector, 
	  class Selector1 = NullSelector,
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class TagMember
{
    static unsigned int OnText(void *product, const std::string& s)
    {
	((T*)product)->*pmem = s;
	return 0;
    }
public:
    typedef T Target;

    AssertCorrectTargetType<Selector0, Target> assert0;
    AssertCorrectTargetType<Selector1, Target> assert1;
    AssertCorrectTargetType<Selector2, Target> assert2;
    AssertCorrectTargetType<Selector3, Target> assert3;
    AssertCorrectTargetType<Selector4, Target> assert4;
    AssertCorrectTargetType<Selector5, Target> assert5;
    AssertCorrectTargetType<Selector6, Target> assert6;
    AssertCorrectTargetType<Selector7, Target> assert7;

    static const internals::Data data;
};


        /* Tag with uint contents (member) */


/** Parse out the integer contents of an XML element; see @ref xml.
 *
 * Stores the contents of an XML element, into a member variable of
 * the target object. @sa TagCallback.
 *
 * @param name      XML name of the element
 * @param T         C++ type of the target object
 * @param pmem      Pointer-to-member of the T object's unsigned int member
 * @param Selector0 Contained tags, if any.
 * @param Selector1 ...
 */
template <const char *name,
	  class T, unsigned int (T::*pmem),
	  class Selector0 = NullSelector, 
	  class Selector1 = NullSelector,
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class TagMemberInt
{
    static unsigned int OnText(void *product, const std::string& s)
    {
	((T*)product)->*pmem = (unsigned int)strtoul(s.c_str(), NULL, 10);
	return 0;
    }
public:
    typedef T Target;

    AssertCorrectTargetType<Selector0, Target> assert0;
    AssertCorrectTargetType<Selector1, Target> assert1;
    AssertCorrectTargetType<Selector2, Target> assert2;
    AssertCorrectTargetType<Selector3, Target> assert3;
    AssertCorrectTargetType<Selector4, Target> assert4;
    AssertCorrectTargetType<Selector5, Target> assert5;
    AssertCorrectTargetType<Selector6, Target> assert6;
    AssertCorrectTargetType<Selector7, Target> assert7;

    static const internals::Data data;
};


        /* Attribute */


/** Parse out a specific attribute of an XML element; see @ref xml.
 *
 * Makes a callback into the target object, whenever a specific attribute of
 * the enclosing xml::Tag is found.
 *
 * @param name     XML name of the attribute
 * @param T        C++ type of the target object
 * @param fn       Pointer-to-member of the T object's callback method
 */
template <const char *name, class T,
	  unsigned int (T::*fn)(const std::string&) >
class Attribute
{
    static unsigned int OnText(void *obs, const std::string& s)
    {
	return (((T*)obs)->*fn)(s);
    }
public:
    typedef T Target;

    static const internals::Data data;
};


        /* Structure */


/** Parse contained XML elements into a struct; see @ref xml.
 *
 * Store (strings or structures corresponding to) contained XML elements, into
 * structure members.
 *
 * @param name      XML name of the container element
 * @param NewTarget C++ type of the struct to become the new target object
 * @param T         C++ type of the parent target object of the new struct
 * @param pmem      Pointer-to-member of the T object's NewTarget member
 * @param Selector0 Contained tags, usually including TagMember.
 * @param Selector1 ...
 */
template <const char *name, class NewTarget,
	  class T, NewTarget (T::*pmem),
	  class Selector0, 
	  class Selector1, 
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class Structure
{
    static void *OnBegin(void *target)
    {
	return (void*) &((T*)target->*pmem);
    }

    AssertCorrectTargetType<Selector0, NewTarget> assert0;
    AssertCorrectTargetType<Selector1, NewTarget> assert1;
    AssertCorrectTargetType<Selector2, NewTarget> assert2;
    AssertCorrectTargetType<Selector3, NewTarget> assert3;
    AssertCorrectTargetType<Selector4, NewTarget> assert4;
    AssertCorrectTargetType<Selector5, NewTarget> assert5;
    AssertCorrectTargetType<Selector6, NewTarget> assert6;
    AssertCorrectTargetType<Selector7, NewTarget> assert7;

public:
    typedef T Target;

    static const internals::Data data;
};


        /* List */


/** Parse a sequence of XML elements into a list; see @ref xml.
 *
 * Store (structures corresponding to) a sequence of similar XML
 * elements, into a std::list.
 *
 * @param name      XML name of the repeating element
 * @param NewTarget C++ type of the new target object for each repeated item
 * @param T         C++ type of the target object in which to create the list
 * @param pmem      Pointer-to-member of the T object's list<NewTarget> member
 * @param Selector0 Contained tags, usually including TagMember.
 * @param Selector1 ...
 */
template <const char *name, class NewTarget,
	  class T, std::list<NewTarget> (T::*pmem),
	  class Selector0, 
	  class Selector1 = NullSelector, 
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class List
{
    static void *OnBegin(void *target)
    {
	NewTarget newt;
	((T*)target->*pmem).push_back(newt);
	return (void*) &(((T*)target->*pmem).back());
    }

    AssertCorrectTargetType<Selector0, NewTarget> assert0;
    AssertCorrectTargetType<Selector1, NewTarget> assert1;
    AssertCorrectTargetType<Selector2, NewTarget> assert2;
    AssertCorrectTargetType<Selector3, NewTarget> assert3;
    AssertCorrectTargetType<Selector4, NewTarget> assert4;
    AssertCorrectTargetType<Selector5, NewTarget> assert5;
    AssertCorrectTargetType<Selector6, NewTarget> assert6;
    AssertCorrectTargetType<Selector7, NewTarget> assert7;

public:
    typedef T Target;

    static const internals::Data data;
};

} // namespace xml

#include "xml_internal.h"

#endif
